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
    /* Add as most recent. */
    if (view->link_recent.prev != view->link_recent.next)
        wl_list_remove(&view->link_recent);
    wl_list_insert(&server->scene.views_recent, &view->link_recent);
}

void
bsi_views_add_minimized(struct bsi_server* server, struct bsi_view* view)
{
    wl_list_insert(&server->scene.views_minimized, &view->link_server);
    wlr_scene_node_set_enabled(view->scene_node, false);
    /* Save the geometry. */
    wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
    wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);
    /* Add as most recent. */
    if (view->link_recent.prev != view->link_recent.next)
        wl_list_remove(&view->link_recent);
    wl_list_insert(&server->scene.views_recent, &view->link_recent);
}

void
bsi_views_mru_focus(struct bsi_server* server)
{
    if (!wl_list_empty(&server->scene.views_recent)) {
        struct bsi_view* view_mru = wl_container_of(
            server->scene.views_recent.prev, view_mru, link_recent);
        bsi_views_remove(view_mru);
        bsi_views_add(server, view_mru);
        bsi_view_focus(view_mru);
    }
}

void
bsi_views_remove(struct bsi_view* view)
{
    wl_list_remove(&view->link_server);
    wl_list_remove(&view->link_recent);
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
    view->state = BSI_VIEW_STATE_NORMAL;

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
    if (prev_focused && prev_focused == view->toplevel->base->surface)
        return;

    /* The surface is not a toplevel surface. */
    if (prev_focused && strcmp(prev_focused->role->name, "xdg_toplevel") == 0) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_focused);
        wlr_xdg_toplevel_set_activated(prev_focused_xdg->toplevel, false);
    }

    /* Move to front of server views. */
    bsi_views_remove(view);
    bsi_views_add(server, view);
    /* Node to top & activate. */
    wlr_scene_node_raise_to_top(view->scene_node);
    wlr_xdg_toplevel_set_activated(view->toplevel, true);
    /* Seat, enter this surface with the keyboard. Leave the pointer. */
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
                           union bsi_view_toplevel_event toplevel_event)
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
        struct wlr_xdg_toplevel_resize_event* event = toplevel_event.resize;
        struct wlr_box box;
        wlr_xdg_surface_get_geometry(view->toplevel->base, &box);

        // TODO: Not sure what is happening here
        double border_x = (view->box.x + box.x) +
                          ((event->edges & WLR_EDGE_RIGHT) ? box.width : 0);
        double border_y = (view->box.y + box.y) +
                          ((event->edges & WLR_EDGE_BOTTOM) ? box.height : 0);

        server->cursor.grab_sx = server->wlr_cursor->x - border_x;
        server->cursor.grab_sy = server->wlr_cursor->y - border_y;
        server->cursor.grab_box = box;
        server->cursor.grab_box.x += view->box.x;
        server->cursor.grab_box.y += view->box.y;
        server->cursor.resize_edges = event->edges;
    }
}

void
bsi_view_set_maximized(struct bsi_view* view, bool maximized)
{
    view->state =
        (maximized) ? BSI_VIEW_STATE_MAXIMIZED : BSI_VIEW_STATE_NORMAL;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Unmaximize view '%s', restore prev", view->toplevel->app_id);
        bsi_view_restore_prev(view);
    } else {
        bsi_debug("Maximize view '%s'", view->toplevel->app_id);

        /* Save the geometry. */
        wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
        wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);

        /* Maximize. */
        struct wlr_box output_box;
        wlr_output_layout_get_box(view->server->wlr_output_layout,
                                  view->parent_workspace->output->output,
                                  &output_box);
        wlr_scene_node_set_position(view->scene_node, 0, 0);
        wlr_xdg_toplevel_set_resizing(view->toplevel, true);
        wlr_xdg_toplevel_set_size(
            view->toplevel, output_box.width, output_box.height);
        wlr_xdg_toplevel_set_resizing(view->toplevel, false);
        wlr_xdg_toplevel_set_maximized(view->toplevel, true);
    }
}

void
bsi_view_set_minimized(struct bsi_view* view, bool minimized)
{
    view->state =
        (minimized) ? BSI_VIEW_STATE_MINIMIZED : BSI_VIEW_STATE_NORMAL;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Unimimize view '%s', restore prev", view->toplevel->app_id);
        bsi_view_restore_prev(view);
        bsi_views_remove(view);
        bsi_views_add(view->server, view);
    } else {
        bsi_debug("Minimize view '%s'", view->toplevel->app_id);
        bsi_views_remove(view);
        bsi_views_add_minimized(view->server, view);

        bsi_views_mru_focus(view->server);
    }
}

void
bsi_view_set_fullscreen(struct bsi_view* view, bool fullscreen)
{
    view->state =
        (fullscreen) ? BSI_VIEW_STATE_FULLSCREEN : BSI_VIEW_STATE_NORMAL;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Unfullscreen view '%s'", view->toplevel->app_id);
        bsi_view_restore_prev(view);
    } else {
        bsi_debug("Fullscreen view '%s'", view->toplevel->app_id);

        /* Save the geometry. */
        wlr_xdg_surface_get_geometry(view->toplevel->base, &view->box);
        wlr_scene_node_coords(view->scene_node, &view->box.x, &view->box.y);

        // TODO: Get rid of decorations to put the entire app fullscreen
        /* Fullscreen. */
        struct wlr_box output_box;
        wlr_output_layout_get_box(view->server->wlr_output_layout,
                                  view->parent_workspace->output->output,
                                  &output_box);
        wlr_scene_node_set_position(view->scene_node, 0, 0);
        wlr_xdg_toplevel_set_resizing(view->toplevel, true);
        wlr_xdg_toplevel_set_size(
            view->toplevel, output_box.width, output_box.height);
        wlr_xdg_toplevel_set_resizing(view->toplevel, false);
        wlr_xdg_toplevel_set_fullscreen(view->toplevel, true);
    }
}

void
bsi_view_restore_prev(struct bsi_view* view)
{
    switch (view->state) {
        case BSI_VIEW_STATE_NORMAL:
        case BSI_VIEW_STATE_MINIMIZED:
            wlr_xdg_toplevel_set_maximized(view->toplevel, false);
            wlr_xdg_toplevel_set_fullscreen(view->toplevel, false);
            break;
        case BSI_VIEW_STATE_MAXIMIZED:
            wlr_xdg_toplevel_set_fullscreen(view->toplevel, false);
            break;
        case BSI_VIEW_STATE_FULLSCREEN:
            wlr_xdg_toplevel_set_maximized(view->toplevel, false);
    }

    bsi_debug("Restoring view position to (%d, %d)", view->box.x, view->box.y);
    bsi_debug(
        "Restoring view size to (%d, %d)", view->box.width, view->box.height);
    wlr_scene_node_set_position(view->scene_node, view->box.x, view->box.y);
    wlr_xdg_toplevel_set_resizing(view->toplevel, true);
    wlr_xdg_toplevel_set_size(
        view->toplevel, view->box.width, view->box.height);
    wlr_xdg_toplevel_set_resizing(view->toplevel, false);
}

/**
 * Handlers
 */

void
handle_xdg_surf_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.destroy);
    struct bsi_workspace* workspace = view->parent_workspace;

    /* The view will already have been unmapped by the time it gets here, no
     * need to call remove. */
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
    if (requested->fullscreen)
        // TODO: Should there be code handling fullscreen_output_destroy, we
        // already handle moving views in workspace code
        bsi_view_set_fullscreen(view, requested->fullscreen);
    else if (requested->maximized)
        bsi_view_set_maximized(view, requested->maximized);
    else if (requested->minimized)
        bsi_view_set_minimized(view, requested->minimized);

    view->mapped = true;
    bsi_views_add(server, view);
    bsi_view_focus(view);
}

void
handle_xdg_surf_unmap(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.unmap);

    view->mapped = false;
    bsi_views_remove(view);
    bsi_views_mru_focus(view->server);
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
}

void
handle_toplvl_request_minimize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_minimize from wlr_xdg_toplevel");

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_minimize);
    struct wlr_xdg_surface* surface = view->toplevel->base;

    /* Again, not sure if this is necessary, only a toplevel surface should be
     * able to request minimize anyway. */
    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;

    bsi_view_set_minimized(view, requested->minimized);
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
    union bsi_view_toplevel_event toplevel_event = { .move = event };

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_view_interactive_begin(view, BSI_CURSOR_MOVE, toplevel_event);
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
    union bsi_view_toplevel_event toplevel_event = { .resize = event };

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_view_interactive_begin(view, BSI_CURSOR_RESIZE, toplevel_event);
}

void
handle_toplvl_request_show_window_menu(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_show_window_menu from wlr_xdg_toplevel");

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_show_window_menu);
    struct wlr_xdg_toplevel_show_window_menu_event* event = data;

    // TODO: Handle show window menu
    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_debug("Should show window menu");
}
