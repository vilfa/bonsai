#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
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
#include "bonsai/desktop/workspace.h"
#include "bonsai/input/cursor.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"

void
bsi_views_add(struct bsi_server* server, struct bsi_view* view)
{
    wl_list_insert(&server->scene.views, &view->link_server);
    wlr_scene_node_set_enabled(view->scene_node, true);
    /* Initialize geometry state. */
    wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
    wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);
}

void
bsi_views_remove(struct bsi_server* server, struct bsi_view* view)
{
    // TODO: Add minimized, maximized, etc states to view. Also check
    // wlr_xdg_surface,...

    if (view->link_server.prev == NULL || view->link_server.next == NULL)
        return;

    wl_list_remove(&view->link_server);
    wlr_scene_node_set_enabled(view->scene_node, false);
}

struct bsi_view*
bsi_view_init(struct bsi_view* view,
              struct bsi_server* server,
              struct wlr_xdg_toplevel* toplevel)
{
    view->server = server;
    view->toplevel = toplevel;
    view->box.x = 0;
    view->box.y = 0;
    view->box.width = 0;
    view->box.height = 0;
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
bsi_view_destroy(struct bsi_view* view)
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
    // wl_list_remove(&view->listen.workspace_active.link);
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
    bsi_views_remove(server, view);
    bsi_views_add(server, view);
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
            (view->box.x + box.x) + ((edges & WLR_EDGE_RIGHT) ? box.width : 0);
        double border_y = (view->box.y + box.y) +
                          ((edges & WLR_EDGE_BOTTOM) ? box.height : 0);
        server->cursor.grab_sx = server->wlr_cursor->x - border_x;
        server->cursor.grab_sy = server->wlr_cursor->y - border_y;

        server->cursor.grab_box = box;
        server->cursor.grab_box.x += view->box.x;
        server->cursor.grab_box.y += view->box.y;

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
        wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
        wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);

        struct bsi_server* server = view->server;
        struct bsi_output* output = view->parent_workspace->output;
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->output, &output_box);
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
        wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
        wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);

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
        wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
        wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);

        // TODO: This should probably get rid of decorations to put the entire
        // app fullscreen
        struct bsi_server* server = view->server;
        struct bsi_output* output = view->parent_workspace->output;
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->output, &output_box);
        wlr_scene_node_set_position(view->scene_node, 0, 0);
        wlr_xdg_toplevel_set_size(
            view->toplevel, output_box.width, output_box.height);
    }
}

void
bsi_view_restore_prev(struct bsi_view* view)
{
    wlr_xdg_toplevel_set_resizing(view->toplevel, true);
    wlr_xdg_toplevel_set_size(
        view->toplevel, view->box.width, view->box.height);
    wlr_xdg_toplevel_set_resizing(view->toplevel, false);

    bsi_debug("Restoring view position to (%d, %d)", view->box.x, view->box.y);
    wlr_scene_node_set_position(view->scene_node, view->box.x, view->box.y);
}

/**
 * Handlers
 */

void
handle_xdg_surf_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.destroy);
    struct bsi_server* server = view->server;
    struct bsi_workspace* workspace = view->parent_workspace;

    bsi_views_remove(server, view);
    bsi_workspace_view_remove(workspace, view);
    bsi_view_destroy(view);

    bsi_info("Workspace %s now has %d views",
             workspace->name,
             wl_list_length(&workspace->views));
}

void
handle_xdg_surf_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.map);
    struct bsi_server* server = view->server;
    struct wlr_xdg_toplevel* toplevel = view->toplevel;

    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;

    if (toplevel->base->added) {
        struct wlr_box wants_box;
        int32_t output_w, output_h;
        wlr_output_effective_resolution(
            view->parent_workspace->output->output, &output_w, &output_h);
        wlr_xdg_surface_get_geometry(toplevel->base, &wants_box);
        wants_box.x = (output_w - wants_box.width) / 2;
        wants_box.y = (output_h - wants_box.height) / 2;
        wlr_scene_node_set_position(view->scene_node, wants_box.x, wants_box.y);

        bsi_debug("Output effective resolution is %dx%d, client wants %dx%d",
                  output_w,
                  output_h,
                  wants_box.width,
                  wants_box.height);

        bsi_debug("Set client node base position to (%d, %d)",
                  wants_box.x,
                  wants_box.y);
    }

    /* Only honor one request of this type. A surface can't request to be
     * maximized and minimized at the same time. */
    if (requested->fullscreen) {
        // TODO: Should there be code handling fullscreen_output_destroy, we
        // already handle moving views in workspace code
        bsi_view_set_fullscreen(view, requested->fullscreen);
        wlr_xdg_toplevel_set_fullscreen(view->toplevel, requested->fullscreen);
        wlr_xdg_surface_schedule_configure(view->toplevel->base);
    } else if (requested->maximized) {
        bsi_view_set_maximized(view, requested->maximized);
        wlr_xdg_toplevel_set_maximized(view->toplevel, requested->maximized);
        wlr_xdg_surface_schedule_configure(view->toplevel->base);
    } else if (requested->minimized) {
        bsi_view_set_minimized(view, requested->minimized);
    }

    view->mapped = true;
    bsi_views_add(server, view);
    bsi_view_focus(view);
}

void
handle_xdg_surf_unmap(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.unmap);
    struct bsi_server* server = view->server;

    view->mapped = false;
    bsi_views_remove(server, view);
}

void
handle_toplvl_request_maximize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_maximize from wlr_xdg_toplevel");

    // TODO: This should probably take into account the panels and such stuff.
    // Also take a look at
    // https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/blob/master/unstable/wlr-layer-shell-unstable-v1.xml

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_maximize);
    struct wlr_xdg_toplevel* toplevel = view->toplevel;
    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;

    bsi_view_set_maximized(view, requested->maximized);
    /* This surface should now consider itself (un-)maximized */
    wlr_xdg_toplevel_set_maximized(toplevel, requested->maximized);
    wlr_xdg_surface_schedule_configure(toplevel->base);
}

void
handle_toplvl_request_fullscreen(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_fullscreen from wlr_xdg_toplevel");

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_fullscreen);
    struct wlr_xdg_toplevel* toplevel = view->toplevel;
    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;

    bsi_view_set_fullscreen(view, requested->fullscreen);
    /* This surface should now consider itself (un-)fullscreen. */
    wlr_xdg_toplevel_set_fullscreen(toplevel, requested->fullscreen);
    wlr_xdg_surface_schedule_configure(toplevel->base);
}

void
handle_toplvl_request_minimize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_minimize from wlr_xdg_toplevel");

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_minimize);
    struct bsi_server* server = view->server;
    struct wlr_xdg_surface* surface = data;

    /* Again, not sure if this is necessary, only a toplevel surface should be
     * able to request minimize anyway. */
    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;

    /* Remove surface from views. */
    bsi_view_set_minimized(view, requested->minimized);
    bsi_views_remove(server, view);
}

void
handle_toplvl_request_move(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_move from wlr_xdg_toplevel");

    /* The user would like to begin an interactive move operation. This is
     * raised when a user clicks on the client side decorations. */
    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_move);
    struct wlr_xdg_toplevel_move_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_view_interactive_begin(view, BSI_CURSOR_MOVE, 0);
}

void
handle_toplvl_request_resize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_resize from wlr_xdg_toplevel");

    /* The user would like to begin an interactive resize operation. This is
     * raised when a use clicks on the client side decorations. */
    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_resize);
    struct wlr_xdg_toplevel_resize_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_view_interactive_begin(view, BSI_CURSOR_RESIZE, event->edges);
}

void
handle_toplvl_request_show_window_menu(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_show_window_menu from wlr_xdg_toplevel");

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_show_window_menu);
    struct wlr_xdg_toplevel_show_window_menu_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_debug("Should show window menu");
    // TODO: Handle show window menu
}
