#include "bonsai/desktop/workspace.h"
#include "bonsai/log.h"
#include <stdint.h>
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/edges.h>

#include "bonsai/desktop/view.h"
#include "bonsai/input/cursor.h"
#include "bonsai/output.h"
#include "bonsai/server.h"

void
bsi_scene_add_view(struct bsi_server* server, struct bsi_view* view)
{
    ++server->scene.len_views;
    wl_list_insert(&server->scene.views, &view->link);

    wlr_scene_node_set_enabled(view->scene_node, true);

    /* Initialize geometry state. */
    struct wlr_box box;
    wlr_xdg_surface_get_geometry(view->toplevel->base, &box);
    view->x = box.x;
    view->y = box.y;
    view->width = box.width;
    view->height = box.height;
}

void
bsi_scene_remove_view(struct bsi_server* server, struct bsi_view* view)
{
    // TODO: Add minimized, maximized, etc states to view. Also check
    // wlr_xdg_surface,...

    if (view->link.prev == NULL || view->link.next == NULL)
        return;

    --server->scene.len_views;
    wl_list_remove(&view->link);

    wlr_scene_node_set_enabled(view->scene_node, false);
}

struct bsi_view*
bsi_view_init(struct bsi_view* view,
              struct bsi_server* server,
              struct wlr_xdg_toplevel* toplevel)
{
    view->server = server;
    view->toplevel = toplevel;
    view->x = 0.0;
    view->y = 0.0;
    view->width = 0;
    view->height = 0;
    view->mapped = false;
    view->maximized = false;
    view->minimized = false;
    view->fullscreen = false;

    /* Create a new node from the root server node. */
    view->scene_node = wlr_scene_xdg_surface_create(
        &view->server->wlr_scene->node, view->toplevel->base);
    view->scene_node->data = view;
    toplevel->base->data = view->scene_node;

    return view;
}

void
bsi_view_finish(struct bsi_view* view)
{
    /* wlr_xdg_surface */
    wl_list_remove(&view->listen.destroy.link);
    wl_list_remove(&view->listen.map.link);
    wl_list_remove(&view->listen.unmap.link);
    /* wlr_xdg_toplevel */
    wl_list_remove(&view->listen.request_maximize.link);
    wl_list_remove(&view->listen.request_fullscreen.link);
    wl_list_remove(&view->listen.request_minimize.link);
    wl_list_remove(&view->listen.request_move.link);
    wl_list_remove(&view->listen.request_resize.link);
    wl_list_remove(&view->listen.request_show_window_menu.link);
    /* bsi_workspaces */
    wl_list_remove(&view->listen.workspace_active.link);
}

void
bsi_view_destroy(struct bsi_view* view)
{
    free(view);
}

void
bsi_view_focus(struct bsi_view* view)
{
    struct bsi_server* server = view->server;
    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_surface* prev_focused = seat->keyboard_state.focused_surface;

    /* The surface is already focused. */
    if (prev_focused == view->toplevel->base->surface)
        return;

    if (prev_focused && strcmp(prev_focused->role->name, "xdg_toplevel") == 0) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_focused);
        assert(prev_focused_xdg->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);
        wlr_xdg_toplevel_set_activated(prev_focused_xdg->toplevel, false);
    }

    /* Move view to top. */
    wlr_scene_node_raise_to_top(view->scene_node);
    /* Add the view to the front of the list. */
    bsi_scene_remove_view(server, view);
    bsi_scene_add_view(server, view);
    /* Activate the view surface. */
    wlr_xdg_toplevel_set_activated(view->toplevel, true);
    /* Tell seat to enter this surface with the keyboard. Don't touch the
     * pointer. */
    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
    wlr_seat_keyboard_notify_enter(seat,
                                   view->toplevel->base->surface,
                                   keyboard->keycodes,
                                   keyboard->num_keycodes,
                                   &keyboard->modifiers);
}

void
bsi_view_interactive_begin(struct bsi_view* view,
                           enum bsi_cursor_mode cursor_mode,
                           uint32_t edges)
{
    /* Sets up an interactive move or resize operation. The compositor consumes
     * these events. */
    struct bsi_server* server = view->server;
    struct wlr_surface* focused_surface =
        server->wlr_seat->pointer_state.focused_surface;

    /* Deny requests from unfocused clients. */
    if (focused_surface == NULL)
        return;

    if (view->toplevel->base->surface !=
        wlr_surface_get_root_surface(focused_surface))
        return;

    server->cursor.grabbed_view = view;
    server->cursor.cursor_mode = cursor_mode;

    if (cursor_mode == BSI_CURSOR_MOVE) {
        /* Set the surface local coordinates for this grab. */
        int32_t lx, ly;
        wlr_scene_node_coords(view->scene_node, &lx, &ly);
        server->cursor.grab_sx = server->wlr_cursor->x - lx;
        server->cursor.grab_sy = server->wlr_cursor->y - ly;

        bsi_debug("Surface local move coords are (%.2f, %.2f)",
                  server->cursor.grab_sx,
                  server->cursor.grab_sy);
    } else {
        struct wlr_box box;
        wlr_xdg_surface_get_geometry(view->toplevel->base, &box);

        // TODO: Not sure what is happening here
        double border_x =
            (view->x + box.x) + ((edges & WLR_EDGE_RIGHT) ? box.width : 0);
        double border_y =
            (view->y + box.y) + ((edges & WLR_EDGE_BOTTOM) ? box.height : 0);
        server->cursor.grab_sx = server->wlr_cursor->x - border_x;
        server->cursor.grab_sy = server->wlr_cursor->y - border_y;

        server->cursor.grab_box = box;
        server->cursor.grab_box.x += view->x;
        server->cursor.grab_box.y += view->y;

        server->cursor.resize_edges = edges;
    }
}

void
bsi_view_set_maximized(struct bsi_view* view, bool maximized)
{
    assert(view);

    view->maximized = maximized;

    if (!maximized) {
        bsi_debug("Set not maximized for view %s, restore prev",
                  view->toplevel->app_id);
        bsi_view_restore_prev(view);
    } else {
        bsi_debug("Set maximized for view %s", view->toplevel->app_id);
        /* Save the geometry. */
        struct wlr_box box;
        int32_t lx, ly;
        wlr_xdg_surface_get_geometry(view->toplevel->base, &box);
        wlr_scene_node_coords(view->scene_node, &lx, &ly);
        view->x = lx;
        view->y = ly;
        view->width = box.width;
        view->height = box.height;

        struct bsi_server* server = view->server;
        struct bsi_output* output = view->parent_workspace->output;
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->wlr_output, &output_box);
        wlr_scene_node_set_position(view->scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            view->toplevel, output_box.width, output_box.height);
    }
}

void
bsi_view_set_minimized(struct bsi_view* view, bool minimized)
{
    assert(view);

    view->minimized = minimized;

    if (!minimized) {
        bsi_debug("Set not minimized for view %s, restore prev",
                  view->toplevel->app_id);
        bsi_view_restore_prev(view);
        wlr_scene_node_set_enabled(view->scene_node, !minimized);
    } else {
        bsi_debug("Set minimized for view %s", view->toplevel->app_id);
        /* Save the geometry. */
        struct wlr_box box;
        int32_t lx, ly;
        wlr_xdg_surface_get_geometry(view->toplevel->base, &box);
        wlr_scene_node_coords(view->scene_node, &lx, &ly);
        view->x = lx;
        view->y = ly;
        view->width = box.width;
        view->height = box.height;
        wlr_scene_node_set_enabled(view->scene_node, false);
    }
}

void
bsi_view_set_fullscreen(struct bsi_view* view, bool fullscreen)
{
    assert(view);

    view->fullscreen = fullscreen;

    if (!fullscreen) {
        bsi_view_restore_prev(view);
    } else {
        /* Save the geometry. */
        struct wlr_box box;
        int32_t lx, ly;
        wlr_xdg_surface_get_geometry(view->toplevel->base, &box);
        wlr_scene_node_coords(view->scene_node, &lx, &ly);
        view->x = lx;
        view->y = ly;
        view->width = box.width;
        view->height = box.height;

        // TODO: This should probably get rid of decorations to put the entire
        // app fullscreen
        struct bsi_server* server = view->server;
        struct bsi_output* output = view->parent_workspace->output;
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->wlr_output, &output_box);
        wlr_scene_node_set_position(view->scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            view->toplevel, output_box.width, output_box.height);
    }
}

void
bsi_view_restore_prev(struct bsi_view* view)
{
    wlr_xdg_toplevel_set_resizing(view->toplevel, true);
    wlr_xdg_toplevel_set_size(view->toplevel, view->width, view->height);
    wlr_xdg_toplevel_set_resizing(view->toplevel, false);

    bsi_debug("Restoring view position to (%.2lf, %.2lf)", view->x, view->y);

    wlr_scene_node_set_position(view->scene_node, view->x, view->y);
}
