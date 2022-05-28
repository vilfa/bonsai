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
bsi_scene_add(struct bsi_server* bsi_server, struct bsi_view* bsi_view)
{
    assert(bsi_server);
    assert(bsi_view);

    ++bsi_server->scene.len;
    wl_list_insert(&bsi_server->scene.views, &bsi_view->link);

    wlr_scene_node_set_enabled(bsi_view->wlr_scene_node, true);

    /* Initialize geometry state. */
    struct wlr_box box;
    wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_toplevel->base, &box);
    bsi_view->x = box.x;
    bsi_view->y = box.y;
    bsi_view->width = box.width;
    bsi_view->height = box.height;
}

void
bsi_scene_remove(struct bsi_server* bsi_server, struct bsi_view* bsi_view)
{
    assert(bsi_server);
    assert(bsi_view);

    // TODO: Add minimized, maximized, etc states to view. Also check
    // wlr_xdg_surface,...

    if (bsi_view->link.prev == NULL || bsi_view->link.next == NULL)
        return;

    --bsi_server->scene.len;
    wl_list_remove(&bsi_view->link);

    wlr_scene_node_set_enabled(bsi_view->wlr_scene_node, false);
}

struct bsi_view*
bsi_view_init(struct bsi_view* bsi_view,
              struct bsi_server* bsi_server,
              struct wlr_xdg_toplevel* wlr_xdg_toplevel,
              struct bsi_workspace* bsi_workspace)
{
    assert(bsi_view);
    assert(bsi_server);
    assert(wlr_xdg_toplevel);
    assert(bsi_workspace);

    bsi_view->bsi_server = bsi_server;
    bsi_view->wlr_xdg_toplevel = wlr_xdg_toplevel;
    bsi_view->bsi_workspace = bsi_workspace;
    bsi_view->x = 0.0;
    bsi_view->y = 0.0;
    bsi_view->width = 0;
    bsi_view->height = 0;
    bsi_view->mapped = false;
    bsi_view->maximized = false;
    bsi_view->minimized = false;
    bsi_view->fullscreen = false;

    /* Create a new node from the root server node. */
    bsi_view->wlr_scene_node =
        wlr_scene_xdg_surface_create(&bsi_view->bsi_server->wlr_scene->node,
                                     bsi_view->wlr_xdg_toplevel->base);
    bsi_view->wlr_scene_node->data = bsi_view;
    wlr_xdg_toplevel->base->data = bsi_view->wlr_scene_node;

    return bsi_view;
}

void
bsi_view_finish(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    /* wlr_xdg_surface */
    wl_list_remove(&bsi_view->listen.destroy_xdg_surface.link);
    wl_list_remove(&bsi_view->listen.map.link);
    wl_list_remove(&bsi_view->listen.unmap.link);
    /* wlr_xdg_toplevel */
    wl_list_remove(&bsi_view->listen.request_maximize.link);
    wl_list_remove(&bsi_view->listen.request_fullscreen.link);
    wl_list_remove(&bsi_view->listen.request_minimize.link);
    wl_list_remove(&bsi_view->listen.request_move.link);
    wl_list_remove(&bsi_view->listen.request_resize.link);
    wl_list_remove(&bsi_view->listen.request_show_window_menu.link);
    /* bsi_workspaces */
    // wl_list_remove(&bsi_view->listen.active_workspace.link);
}

void
bsi_view_destroy(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    free(bsi_view);
}

void
bsi_view_focus(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    struct bsi_server* bsi_server = bsi_view->bsi_server;
    struct wlr_seat* wlr_seat = bsi_server->wlr_seat;
    struct wlr_surface* prev_focused = wlr_seat->keyboard_state.focused_surface;

    /* The surface is already focused. */
    if (prev_focused == bsi_view->wlr_xdg_toplevel->base->surface)
        return;

    if (prev_focused) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_focused);
        assert(prev_focused_xdg->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);
        wlr_xdg_toplevel_set_activated(prev_focused_xdg->toplevel, false);
    }

    /* Move view to top. */
    wlr_scene_node_raise_to_top(bsi_view->wlr_scene_node);
    /* Add the view to the front of the list. */
    bsi_scene_remove(bsi_server, bsi_view);
    bsi_scene_add(bsi_server, bsi_view);
    /* Activate the view surface. */
    wlr_xdg_toplevel_set_activated(bsi_view->wlr_xdg_toplevel, true);
    /* Tell seat to enter this surface with the keyboard. Don't touch the
     * pointer. */
    struct wlr_keyboard* wlr_keyboard = wlr_seat_get_keyboard(wlr_seat);
    wlr_seat_keyboard_notify_enter(wlr_seat,
                                   bsi_view->wlr_xdg_toplevel->base->surface,
                                   wlr_keyboard->keycodes,
                                   wlr_keyboard->num_keycodes,
                                   &wlr_keyboard->modifiers);
}

void
bsi_view_interactive_begin(struct bsi_view* bsi_view,
                           enum bsi_cursor_mode bsi_cursor_mode,
                           uint32_t edges)
{
    assert(bsi_view);

    /* Sets up an interactive move or resize operation. The compositor consumes
     * these events. */
    struct bsi_server* bsi_server = bsi_view->bsi_server;
    struct wlr_surface* focused_surface =
        bsi_server->wlr_seat->pointer_state.focused_surface;

    /* Deny requests from unfocused clients. */
    if (bsi_view->wlr_xdg_toplevel->base->surface !=
        wlr_surface_get_root_surface(focused_surface))
        return;

    bsi_server->cursor.grabbed_view = bsi_view;
    bsi_server->cursor.cursor_mode = bsi_cursor_mode;

    if (bsi_cursor_mode == BSI_CURSOR_MOVE) {
        // TODO: Not sure what is happening here
        bsi_server->cursor.grab_x = bsi_server->wlr_cursor->x - bsi_view->x;
        bsi_server->cursor.grab_y = bsi_server->wlr_cursor->y - bsi_view->y;
    } else {
        struct wlr_box box;
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_toplevel->base, &box);

        // TODO: Not sure what is happening here
        double border_x =
            (bsi_view->x + box.x) + ((edges & WLR_EDGE_RIGHT) ? box.width : 0);
        double border_y = (bsi_view->y + box.y) +
                          ((edges & WLR_EDGE_BOTTOM) ? box.height : 0);
        bsi_server->cursor.grab_x = bsi_server->wlr_cursor->x - border_x;
        bsi_server->cursor.grab_y = bsi_server->wlr_cursor->y - border_y;

        bsi_server->cursor.grab_box = box;
        bsi_server->cursor.grab_box.x += bsi_view->x;
        bsi_server->cursor.grab_box.y += bsi_view->y;

        bsi_server->cursor.resize_edges = edges;
    }
}

void
bsi_view_set_maximized(struct bsi_view* bsi_view, bool maximized)
{
    assert(bsi_view);

    bsi_view->maximized = maximized;

    if (!maximized) {
        bsi_view_restore_prev(bsi_view);
    } else {
        /* Save the geometry. */
        struct wlr_box box;
        int32_t lx, ly;
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_toplevel->base, &box);
        wlr_scene_node_coords(bsi_view->wlr_scene_node, &lx, &ly);
        bsi_view->x = lx;
        bsi_view->y = ly;
        bsi_view->width = box.width;
        bsi_view->height = box.height;

        struct bsi_server* server = bsi_view->bsi_server;
        struct bsi_output* output = bsi_view->bsi_workspace->bsi_output;
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->wlr_output, &output_box);
        wlr_scene_node_set_position(bsi_view->wlr_scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            bsi_view->wlr_xdg_toplevel, output_box.width, output_box.height);
    }
}

void
bsi_view_set_minimized(struct bsi_view* bsi_view, bool minimized)
{
    assert(bsi_view);

    bsi_view->minimized = minimized;

    if (!minimized) {
        bsi_view_restore_prev(bsi_view);
        wlr_scene_node_set_enabled(bsi_view->wlr_scene_node, !minimized);
    } else {
        /* Save the geometry. */
        struct wlr_box box;
        int32_t lx, ly;
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_toplevel->base, &box);
        wlr_scene_node_coords(bsi_view->wlr_scene_node, &lx, &ly);
        bsi_view->x = lx;
        bsi_view->y = ly;
        bsi_view->width = box.width;
        bsi_view->height = box.height;

        wlr_scene_node_set_enabled(bsi_view->wlr_scene_node, false);
    }
}

void
bsi_view_set_fullscreen(struct bsi_view* bsi_view, bool fullscreen)
{
    assert(bsi_view);

    bsi_view->fullscreen = fullscreen;

    if (!fullscreen) {
        bsi_view_restore_prev(bsi_view);
    } else {
        /* Save the geometry. */
        struct wlr_box box;
        int32_t lx, ly;
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_toplevel->base, &box);
        wlr_scene_node_coords(bsi_view->wlr_scene_node, &lx, &ly);
        bsi_view->x = lx;
        bsi_view->y = ly;
        bsi_view->width = box.width;
        bsi_view->height = box.height;

        // TODO: This should probably get rid of decorations to put the entire
        // app fullscreen
        struct bsi_server* server = bsi_view->bsi_server;
        struct bsi_output* output = bsi_view->bsi_workspace->bsi_output;
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->wlr_output, &output_box);
        wlr_scene_node_set_position(bsi_view->wlr_scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            bsi_view->wlr_xdg_toplevel, output_box.width, output_box.height);
    }
}

void
bsi_view_restore_prev(struct bsi_view* bsi_view)
{
    wlr_xdg_toplevel_set_resizing(bsi_view->wlr_xdg_toplevel, true);
    wlr_xdg_toplevel_set_size(
        bsi_view->wlr_xdg_toplevel, bsi_view->width, bsi_view->height);
    wlr_xdg_toplevel_set_resizing(bsi_view->wlr_xdg_toplevel, false);
    wlr_scene_node_set_position(
        bsi_view->wlr_scene_node, bsi_view->x, bsi_view->y);
}
