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

#include "bonsai/config/output.h"
#include "bonsai/desktop/view.h"
#include "bonsai/input/cursor.h"
#include "bonsai/server.h"

struct bsi_views*
bsi_views_init(struct bsi_views* bsi_views, struct bsi_server* bsi_server)
{
    assert(bsi_views);
    assert(bsi_server);

    bsi_views->len = 0;
    bsi_views->bsi_server = bsi_server;
    wl_list_init(&bsi_views->views);

    return bsi_views;
}

void
bsi_views_add(struct bsi_views* bsi_views, struct bsi_view* bsi_view)
{
    assert(bsi_views);
    assert(bsi_view);

    ++bsi_views->len;
    wl_list_insert(&bsi_views->views, &bsi_view->link);

    wlr_scene_node_set_enabled(bsi_view->wlr_scene_node, true);

    /* Initialize geometry state. */
    struct wlr_box box;
    wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_surface, &box);
    bsi_view->x = box.x;
    bsi_view->y = box.y;
    bsi_view->width = box.width;
    bsi_view->height = box.height;
}

void
bsi_views_remove(struct bsi_views* bsi_views, struct bsi_view* bsi_view)
{
    assert(bsi_views);
    assert(bsi_view);

    // TODO: Add minimized, maximized, etc states to view. Also check
    // wlr_xdg_surface,...

    if (bsi_view->link.prev == NULL || bsi_view->link.next == NULL)
        return;

    --bsi_views->len;
    wl_list_remove(&bsi_view->link);

    wlr_scene_node_set_enabled(bsi_view->wlr_scene_node, false);
}

struct bsi_view*
bsi_view_init(struct bsi_view* bsi_view,
              struct bsi_server* bsi_server,
              struct wlr_xdg_surface* wlr_xdg_surface,
              struct bsi_workspace* bsi_workspace)
{
    assert(bsi_view);
    assert(bsi_server);
    assert(wlr_xdg_surface);
    assert(bsi_workspace);

    bsi_view->bsi_server = bsi_server;
    bsi_view->wlr_xdg_surface = wlr_xdg_surface;
    bsi_view->bsi_workspace = bsi_workspace;
    bsi_view->x = 0.0;
    bsi_view->y = 0.0;
    bsi_view->width = 0;
    bsi_view->height = 0;
    bsi_view->len_active_listen = 0;
    bsi_view->mapped = false;
    bsi_view->maximized = false;
    bsi_view->minimized = false;
    bsi_view->fullscreen = false;

    /* Create a new node from the root server node. */
    bsi_view->wlr_scene_node = wlr_scene_xdg_surface_create(
        &bsi_view->bsi_server->wlr_scene->node, bsi_view->wlr_xdg_surface);
    bsi_view->wlr_scene_node->data = bsi_view;
    wlr_xdg_surface->data = bsi_view->wlr_scene_node;

    return bsi_view;
}

void
bsi_view_finish(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    /* wlr_xdg_surface */
    wl_list_remove(&bsi_view->listen.destroy_xdg_surface.link);
    wl_list_remove(&bsi_view->listen.ping_timeout.link);
    wl_list_remove(&bsi_view->listen.new_popup.link);
    wl_list_remove(&bsi_view->listen.map.link);
    wl_list_remove(&bsi_view->listen.unmap.link);
    wl_list_remove(&bsi_view->listen.configure.link);
    wl_list_remove(&bsi_view->listen.ack_configure.link);
    /* wlr_xdg_toplevel */
    wl_list_remove(&bsi_view->listen.request_maximize.link);
    wl_list_remove(&bsi_view->listen.request_fullscreen.link);
    wl_list_remove(&bsi_view->listen.request_minimize.link);
    wl_list_remove(&bsi_view->listen.request_move.link);
    wl_list_remove(&bsi_view->listen.request_resize.link);
    wl_list_remove(&bsi_view->listen.request_show_window_menu.link);
    wl_list_remove(&bsi_view->listen.set_parent.link);
    wl_list_remove(&bsi_view->listen.set_title.link);
    wl_list_remove(&bsi_view->listen.set_app_id.link);
    /* wlr_scene_node */
    wl_list_remove(&bsi_view->listen.destroy_scene_node.link);
    /* bsi_workspaces */
    wl_list_remove(&bsi_view->listen.active_workspace.link);
    bsi_view->len_active_listen = 0;
}

void
bsi_view_destroy(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    free(bsi_view->app_id);
    free(bsi_view->app_title);
    free(bsi_view);
}

void
bsi_view_focus(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    struct bsi_server* bsi_server = bsi_view->bsi_server;
    struct bsi_views* bsi_views = &bsi_server->bsi_views;
    struct wlr_seat* wlr_seat = bsi_server->wlr_seat;
    struct wlr_surface* prev_focused = wlr_seat->keyboard_state.focused_surface;

    /* The surface is already focused. */
    if (prev_focused == bsi_view->wlr_xdg_surface->surface)
        return;

    if (prev_focused) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_focused);
        wlr_xdg_toplevel_set_activated(prev_focused_xdg, false);
    }

    /* Move view to top. */
    wlr_scene_node_raise_to_top(bsi_view->wlr_scene_node);
    /* Add the view to the front of the list. */
    bsi_views_remove(bsi_views, bsi_view);
    bsi_views_add(bsi_views, bsi_view);
    /* Activate the view surface. */
    wlr_xdg_toplevel_set_activated(bsi_view->wlr_xdg_surface, true);
    /* Tell seat to enter this surface with the keyboard. Don't touch the
     * pointer. */
    struct wlr_keyboard* wlr_keyboard = wlr_seat_get_keyboard(wlr_seat);
    wlr_seat_keyboard_notify_enter(wlr_seat,
                                   bsi_view->wlr_xdg_surface->surface,
                                   wlr_keyboard->keycodes,
                                   wlr_keyboard->num_keycodes,
                                   &wlr_keyboard->modifiers);
}

void
bsi_view_set_app_id(struct bsi_view* bsi_view, const char* app_id)
{
    assert(bsi_view);
    assert(app_id);

    bsi_view->app_id = strdup(app_id);
}

void
bsi_view_set_app_title(struct bsi_view* bsi_view, const char* app_title)
{
    assert(bsi_view);
    assert(app_title);

    bsi_view->app_title = strdup(app_title);
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
    if (bsi_view->wlr_xdg_surface->surface !=
        wlr_surface_get_root_surface(focused_surface))
        return;

    bsi_server->bsi_cursor.grabbed_view = bsi_view;
    bsi_server->bsi_cursor.cursor_mode = bsi_cursor_mode;

    if (bsi_cursor_mode == BSI_CURSOR_MOVE) {
        // TODO: Not sure what is happening here
        bsi_server->bsi_cursor.grab_x = bsi_server->wlr_cursor->x - bsi_view->x;
        bsi_server->bsi_cursor.grab_y = bsi_server->wlr_cursor->y - bsi_view->y;
    } else {
        struct wlr_box box;
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_surface, &box);

        // TODO: Not sure what is happening here
        double border_x =
            (bsi_view->x + box.x) + ((edges & WLR_EDGE_RIGHT) ? box.width : 0);
        double border_y = (bsi_view->y + box.y) +
                          ((edges & WLR_EDGE_BOTTOM) ? box.height : 0);
        bsi_server->bsi_cursor.grab_x = bsi_server->wlr_cursor->x - border_x;
        bsi_server->bsi_cursor.grab_y = bsi_server->wlr_cursor->y - border_y;

        bsi_server->bsi_cursor.grab_geobox = box;
        bsi_server->bsi_cursor.grab_geobox.x += bsi_view->x;
        bsi_server->bsi_cursor.grab_geobox.y += bsi_view->y;

        bsi_server->bsi_cursor.resize_edges = edges;
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
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_surface, &box);
        wlr_scene_node_coords(bsi_view->wlr_scene_node, &lx, &ly);
        bsi_view->x = lx;
        bsi_view->y = ly;
        bsi_view->width = box.width;
        bsi_view->height = box.height;

        struct bsi_server* server = bsi_view->bsi_server;
        struct bsi_output* output = bsi_view->bsi_workspace->bsi_output;
        struct wlr_box* output_box = wlr_output_layout_get_box(
            server->wlr_output_layout, output->wlr_output);
        wlr_scene_node_set_position(bsi_view->wlr_scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            bsi_view->wlr_xdg_surface, output_box->width, output_box->height);
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
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_surface, &box);
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
        wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_surface, &box);
        wlr_scene_node_coords(bsi_view->wlr_scene_node, &lx, &ly);
        bsi_view->x = lx;
        bsi_view->y = ly;
        bsi_view->width = box.width;
        bsi_view->height = box.height;

        // TODO: This should probably get rid of decorations to put the entire
        // app fullscreen
        struct bsi_server* server = bsi_view->bsi_server;
        struct bsi_output* output = bsi_view->bsi_workspace->bsi_output;
        struct wlr_box* output_box = wlr_output_layout_get_box(
            server->wlr_output_layout, output->wlr_output);
        wlr_scene_node_set_position(bsi_view->wlr_scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            bsi_view->wlr_xdg_surface, output_box->width, output_box->height);
    }
}

void
bsi_view_restore_prev(struct bsi_view* bsi_view)
{
    wlr_xdg_toplevel_set_resizing(bsi_view->wlr_xdg_surface, true);
    wlr_xdg_toplevel_set_size(
        bsi_view->wlr_xdg_surface, bsi_view->width, bsi_view->height);
    wlr_xdg_toplevel_set_resizing(bsi_view->wlr_xdg_surface, false);
    wlr_scene_node_set_position(
        bsi_view->wlr_scene_node, bsi_view->x, bsi_view->y);
}

void
bsi_view_listener_add(struct bsi_view* bsi_view,
                      struct wl_listener* bsi_listener_memb,
                      struct wl_signal* bsi_signal_memb,
                      wl_notify_func_t func)
{
    assert(bsi_view);
    assert(func);

    bsi_listener_memb->notify = func;
    ++bsi_view->len_active_listen;

    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}
