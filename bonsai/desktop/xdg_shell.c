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
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/edges.h>

#include "bonsai/desktop/idle.h"
#include "bonsai/desktop/view.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

/* Implementation. */
static struct bsi_xdg_shell_view*
xdg_shell_view_from_view(struct bsi_view* view)
{
    return (struct bsi_xdg_shell_view*)view;
}

static void
xdg_shell_view_destroy(struct bsi_view* view)
{
    struct bsi_xdg_shell_view* v = xdg_shell_view_from_view(view);

    wl_list_remove(&v->listen.map.link);
    wl_list_remove(&v->listen.unmap.link);
    wl_list_remove(&v->listen.destroy.link);
    wl_list_remove(&v->listen.request_maximize.link);
    wl_list_remove(&v->listen.request_fullscreen.link);
    wl_list_remove(&v->listen.request_minimize.link);
    wl_list_remove(&v->listen.request_move.link);
    wl_list_remove(&v->listen.request_resize.link);
    wl_list_remove(&v->listen.request_show_window_menu.link);

    free(view);
}

static void
xdg_shell_view_focus(struct bsi_view* view)
{
    struct bsi_server* server = view->server;
    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_surface* prev_keyboard = seat->keyboard_state.focused_surface;

    /* The surface is already focused. */
    if (prev_keyboard && prev_keyboard == view->wlr_xdg_toplevel->base->surface)
        return;

    /* Deactivate the previously focused surface and notify the client. */
    if (prev_keyboard && wlr_surface_is_xdg_surface(prev_keyboard)) {
        struct wlr_xdg_surface* prev_focused =
            wlr_xdg_surface_from_wlr_surface(prev_keyboard);
        wlr_xdg_toplevel_set_activated(prev_focused->toplevel, false);
    }

    /* Move to front of server views. */
    views_remove(view);
    views_add(server, view);

    /* Node to top & activate. */
    wlr_scene_node_raise_to_top(view->node);
    wlr_xdg_toplevel_set_activated(view->wlr_xdg_toplevel, true);

    /* Seat, enter this surface with the keyboard. Leave the pointer. */
    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
    wlr_seat_keyboard_notify_enter(seat,
                                   view->wlr_xdg_toplevel->base->surface,
                                   keyboard->keycodes,
                                   keyboard->num_keycodes,
                                   &keyboard->modifiers);

    /* Arrange output layers. */
    output_layers_arrange(view->workspace->output);
}

static void
xdg_shell_view_cursor_interactive(struct bsi_view* view,
                                  enum bsi_cursor_mode cursor_mode,
                                  union bsi_xdg_toplevel_event toplevel_event)
{
    /* Sets up an interactive move or resize operation. The compositor consumes
     * these events. */
    struct bsi_server* server = view->server;
    struct wlr_surface* focused_surface =
        server->wlr_seat->pointer_state.focused_surface;

    /* Deny requests from unfocused clients. */
    if (focused_surface == NULL)
        return;

    /* Already focused. */
    if (view->wlr_xdg_toplevel->base->surface !=
        wlr_surface_get_root_surface(focused_surface))
        return;

    server->cursor.grabbed_view = view;
    server->cursor.cursor_mode = cursor_mode;

    if (cursor_mode == BSI_CURSOR_MOVE) {
        /* Set the surface local coordinates for this grab. */
        int32_t lx, ly;
        wlr_scene_node_coords(view->node, &lx, &ly);
        server->cursor.grab_sx = server->wlr_cursor->x - lx;
        server->cursor.grab_sy = server->wlr_cursor->y - ly;
        bsi_debug("Surface local move coords are (%.2f, %.2f)",
                  server->cursor.grab_sx,
                  server->cursor.grab_sy);
    } else {
        struct wlr_box surface_box;
        struct wlr_xdg_toplevel_resize_event* event = toplevel_event.resize;
        wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base,
                                     &surface_box);

        /* Get the layout local coordinates of grabbed edge. Subtract the box
         * geometry x & y to account for possible subsurfaces in the form of
         * client side decorations. These might be negative, so subtract to get
         * the right sign.*/
        double edge_lx, edge_ly;
        edge_lx = (view->geom.x + surface_box.x) +
                  ((event->edges & WLR_EDGE_RIGHT) ? surface_box.width : 0);
        edge_ly = (view->geom.y + surface_box.y) +
                  ((event->edges & WLR_EDGE_BOTTOM) ? surface_box.height : 0);

        /* Set the surface local coords of this grab. */
        server->cursor.grab_sx = server->wlr_cursor->x - edge_lx;
        server->cursor.grab_sy = server->wlr_cursor->y - edge_ly;
        /* Set the grab limits. */
        server->cursor.grab_box = surface_box;
        server->cursor.grab_box.x += view->geom.x;
        server->cursor.grab_box.y += view->geom.y;
        server->cursor.grab_box.width += surface_box.x;
        server->cursor.grab_box.height += surface_box.y;
        server->cursor.resize_edges = event->edges;

        bsi_debug("Surface local resize coords are (%.2f, %.2f)",
                  server->cursor.grab_sx,
                  server->cursor.grab_sy);
        bsi_debug("Surface grab box is [%d, %d, %d, %d]",
                  server->cursor.grab_box.x,
                  server->cursor.grab_box.y,
                  server->cursor.grab_box.width,
                  server->cursor.grab_box.height);
    }
}

static void
xdg_shell_view_set_maximized(struct bsi_view* view, bool maximized)
{
    enum bsi_view_state new_state =
        (maximized) ? BSI_VIEW_STATE_MAXIMIZED : BSI_VIEW_STATE_NORMAL;

    if (view->state == new_state)
        return;

    view->state = new_state;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Unmaximize view '%s', restore prev",
                  view->wlr_xdg_toplevel->app_id);
        view_restore_prev(view);
    } else {
        bsi_debug("Maximize view '%s'", view->wlr_xdg_toplevel->app_id);

        /* Save the geometry. */
        wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base, &view->geom);
        wlr_scene_node_coords(view->node, &view->geom.x, &view->geom.y);

        /* Maximize. */
        struct wlr_box* usable_box = &view->workspace->output->usable;
        wlr_scene_node_set_position(view->node, usable_box->x, usable_box->y);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, true);
        wlr_xdg_toplevel_set_size(
            view->wlr_xdg_toplevel, usable_box->width, usable_box->height);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, false);
        wlr_xdg_toplevel_set_maximized(view->wlr_xdg_toplevel, true);
    }
}

static void
xdg_shell_view_set_minimized(struct bsi_view* view, bool minimized)
{
    enum bsi_view_state new_state =
        (minimized) ? BSI_VIEW_STATE_MINIMIZED : BSI_VIEW_STATE_NORMAL;

    if (view->state == new_state)
        return;

    view->state = new_state;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Unimimize view '%s', restore prev",
                  view->wlr_xdg_toplevel->app_id);
        view_restore_prev(view);
        views_remove(view);
        views_add(view->server, view);
        output_layers_arrange(view->workspace->output);
        wlr_scene_node_set_enabled(view->node, true);
    } else {
        bsi_debug("Minimize view '%s'", view->wlr_xdg_toplevel->app_id);
        wlr_scene_node_set_enabled(view->node, false);
        views_remove(view);
        views_focus_recent(view->server);
        views_add(view->server, view);
        output_layers_arrange(view->workspace->output);
    }
}

static void
xdg_shell_view_set_fullscreen(struct bsi_view* view, bool fullscreen)
{
    enum bsi_view_state new_state =
        (fullscreen) ? BSI_VIEW_STATE_FULLSCREEN : BSI_VIEW_STATE_NORMAL;

    if (view->state == new_state)
        return;

    view->state = new_state;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Unfullscreen view '%s'", view->wlr_xdg_toplevel->app_id);

        /* Remove from fullscreen views. */
        wl_list_remove(&view->link_fullscreen);

        /* Remove fullscreen idle inhibitor. */
        idle_inhibitors_remove(view->inhibit.fullscreen);
        idle_inhibitor_destroy(view->inhibit.fullscreen);
        view->inhibit.fullscreen = NULL;
        idle_inhibitors_update(view->server);

        /* Restore previous (incl. decoration mode). */
        wlr_xdg_toplevel_decoration_v1_set_mode(
            view->decoration->xdg_decoration,
            /* view->xdg_decoration_mode */ // TODO
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
        view_restore_prev(view);
    } else {
        bsi_debug("Fullscreen view '%s'", view->wlr_xdg_toplevel->app_id);

        /* Save the geometry. */
        wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base, &view->geom);
        wlr_scene_node_coords(view->node, &view->geom.x, &view->geom.y);

        /* Add to fullscreen views. */
        wl_list_insert(&view->server->scene.views_fullscreen,
                       &view->link_fullscreen);

        /* Add fullscreen idle inhibitor. */
        struct bsi_idle_inhibitor* idle =
            calloc(1, sizeof(struct bsi_idle_inhibitor));
        idle_inhibitor_init(
            idle, NULL, view->server, view, BSI_IDLE_INHIBIT_FULLSCREEN);
        idle_inhibitors_add(view->server, idle);
        view->inhibit.fullscreen = idle;
        idle_inhibitors_update(view->server);

        /* SSD, server doesn't display deco for fullscreen & fullscreen. */
        wlr_xdg_toplevel_decoration_v1_set_mode(
            view->decoration->xdg_decoration,
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        // WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE);
        struct wlr_box output_box;
        wlr_output_layout_get_box(view->server->wlr_output_layout,
                                  view->workspace->output->output,
                                  &output_box);
        wlr_scene_node_set_position(view->node, 0, 0);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, true);
        wlr_xdg_toplevel_set_size(
            view->wlr_xdg_toplevel, output_box.width, output_box.height);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, false);
        wlr_xdg_toplevel_set_fullscreen(view->wlr_xdg_toplevel, true);
    }

    output_layers_arrange(view->workspace->output);
}

static void
xdg_shell_view_set_tiled_left(struct bsi_view* view, bool tiled)
{
    enum bsi_view_state new_state =
        (tiled) ? BSI_VIEW_STATE_TILED_LEFT : BSI_VIEW_STATE_NORMAL;

    if (view->state == new_state)
        return;

    view->state = new_state;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Untile view '%s'", view->wlr_xdg_toplevel->app_id);
        view_restore_prev(view);
    } else {
        bsi_debug("Tile view '%s' left", view->wlr_xdg_toplevel->app_id);

        /* Save the geometry. */
        wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base, &view->geom);
        wlr_scene_node_coords(view->node, &view->geom.x, &view->geom.y);

        /* Tile left. */
        struct wlr_box* usable_box = &view->workspace->output->usable;
        wlr_scene_node_set_position(view->node, usable_box->x, usable_box->y);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, true);
        wlr_xdg_toplevel_set_size(
            view->wlr_xdg_toplevel, usable_box->width / 2, usable_box->height);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, false);
    }
}

static void
xdg_shell_view_set_tiled_right(struct bsi_view* view, bool tiled)
{
    enum bsi_view_state new_state =
        (tiled) ? BSI_VIEW_STATE_TILED_RIGHT : BSI_VIEW_STATE_NORMAL;

    if (view->state == new_state)
        return;

    view->state = new_state;

    if (view->state == BSI_VIEW_STATE_NORMAL) {
        bsi_debug("Untile view '%s'", view->wlr_xdg_toplevel->app_id);
        view_restore_prev(view);
    } else {
        bsi_debug("Tile view '%s' right", view->wlr_xdg_toplevel->app_id);

        /* Save the geometry. */
        wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base, &view->geom);
        wlr_scene_node_coords(view->node, &view->geom.x, &view->geom.y);

        /* Tile right. */
        struct wlr_box* usable_box = &view->workspace->output->usable;
        wlr_scene_node_set_position(
            view->node, usable_box->width / 2, usable_box->y);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, true);
        wlr_xdg_toplevel_set_size(
            view->wlr_xdg_toplevel, usable_box->width / 2, usable_box->height);
        wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, false);
    }
}

static void
xdg_shell_view_restore_prev(struct bsi_view* view)
{
    switch (view->state) {
        case BSI_VIEW_STATE_NORMAL:
        case BSI_VIEW_STATE_MINIMIZED:
            wlr_xdg_toplevel_set_maximized(view->wlr_xdg_toplevel, false);
            wlr_xdg_toplevel_set_fullscreen(view->wlr_xdg_toplevel, false);
            break;
        case BSI_VIEW_STATE_MAXIMIZED:
            wlr_xdg_toplevel_set_fullscreen(view->wlr_xdg_toplevel, false);
            break;
        case BSI_VIEW_STATE_FULLSCREEN:
            wlr_xdg_toplevel_set_maximized(view->wlr_xdg_toplevel, false);
            break;
        default:
            break;
    }

    bsi_debug(
        "Restoring view position to (%d, %d)", view->geom.x, view->geom.y);
    bsi_debug(
        "Restoring view size to (%d, %d)", view->geom.width, view->geom.height);
    wlr_scene_node_set_position(view->node, view->geom.x, view->geom.y);
    wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, true);
    wlr_xdg_toplevel_set_size(
        view->wlr_xdg_toplevel, view->geom.width, view->geom.height);
    wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, false);
}

static bool
xdg_shell_view_intersects(struct bsi_view* view, struct wlr_box* box)
{
    if (view->geom.x < box->x ||
        view->geom.x + view->geom.width > box->x + box->width ||
        view->geom.y < box->y ||
        view->geom.y + view->geom.height > box->y + box->height) {
        return true;
    }
    return false;
}

static void
xdg_shell_view_get_correct(struct bsi_view* view,
                           struct wlr_box* box,
                           struct wlr_box* correction)
{
    if (view->geom.x <= box->x) {
        correction->x += box->x - view->geom.x + 1;
    }
    if (view->geom.x + view->geom.width >= box->x + box->width) {
        int32_t corr =
            (box->x + box->width) - (view->geom.x + view->geom.width) - 1;
        if (view->geom.x + corr > box->x)
            correction->x += corr;
        else
            correction->width += corr;
    }
    if (view->geom.y <= box->y) {
        correction->y += box->y - view->geom.y + 1;
    }
    if (view->geom.y + view->geom.height >= box->y + box->height) {
        int32_t corr =
            (box->y + box->height) - (view->geom.y + view->geom.height) - 1;
        if (view->geom.y + corr > box->y)
            correction->y += corr;
        else
            correction->width += corr;
    }
}

static void
xdg_shell_view_set_correct(struct bsi_view* view, struct wlr_box* correction)
{
    view->geom.x += correction->x;
    view->geom.y += correction->y;
    view->geom.width += correction->width;
    view->geom.height += correction->height;
    wlr_scene_node_set_position(view->node, view->geom.x, view->geom.y);
    wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, true);
    wlr_xdg_toplevel_set_size(
        view->wlr_xdg_toplevel, view->geom.width, view->geom.height);
    wlr_xdg_toplevel_set_resizing(view->wlr_xdg_toplevel, false);
}

static void
xdg_shell_view_request_activate(struct bsi_view* view)
{
    view_focus(view);
}

static const struct bsi_view_impl view_impl = {
    .destroy = xdg_shell_view_destroy,
    .focus = xdg_shell_view_focus,
    .cursor_interactive = xdg_shell_view_cursor_interactive,
    .set_maximized = xdg_shell_view_set_maximized,
    .set_minimized = xdg_shell_view_set_minimized,
    .set_fullscreen = xdg_shell_view_set_fullscreen,
    .set_tiled_left = xdg_shell_view_set_tiled_left,
    .set_tiled_right = xdg_shell_view_set_tiled_right,
    .restore_prev = xdg_shell_view_restore_prev,
    .intersects = xdg_shell_view_intersects,
    .get_correct = xdg_shell_view_get_correct,
    .set_correct = xdg_shell_view_set_correct,
    .request_activate = xdg_shell_view_request_activate,
};

/* Handlers. */
static void
handle_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_xdg_surface");

    struct bsi_xdg_shell_view* v = wl_container_of(listener, v, listen.destroy);
    struct bsi_view* view = &v->view;

    bsi_info("Workspace %s now has %d views",
             view->workspace->name,
             wl_list_length(&view->workspace->views) - 1);

    /* The view will already have been unmapped by the time it gets here, no
     * need to call remove. */
    workspace_view_remove(view->workspace, view);
    view_destroy(view);
}

static void
handle_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from wlr_xdg_surface");

    struct bsi_xdg_shell_view* v = wl_container_of(listener, v, listen.map);
    struct bsi_view* view = &v->view;
    struct bsi_server* server = view->server;
    struct wlr_xdg_toplevel* toplevel = view->wlr_xdg_toplevel;
    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;

    if (toplevel->base->added) {
        struct wlr_box wants_box;
        int32_t output_w, output_h;
        wlr_output_effective_resolution(
            view->workspace->output->output, &output_w, &output_h);
        wlr_xdg_surface_get_geometry(toplevel->base, &wants_box);
        wants_box.x = (output_w - wants_box.width) / 2;
        wants_box.y = (output_h - wants_box.height) / 2;
        wlr_scene_node_set_position(view->node, wants_box.x, wants_box.y);

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
        view_set_fullscreen(view, requested->fullscreen);
    else if (requested->maximized)
        view_set_maximized(view, requested->maximized);
    else if (requested->minimized)
        view_set_minimized(view, requested->minimized);

    view->mapped = true;
    views_add(server, view);
    view_focus(view);
}

static void
handle_unmap(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from wlr_xdg_surface");

    struct bsi_xdg_shell_view* v = wl_container_of(listener, v, listen.unmap);
    struct bsi_view* view = &v->view;

    view->mapped = false;
    views_remove(view);
    views_focus_recent(view->server);
    output_layers_arrange(view->workspace->output);
}

static void
handle_request_maximize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_maximize from wlr_xdg_toplevel");

    struct bsi_xdg_shell_view* v =
        wl_container_of(listener, v, listen.request_maximize);
    struct bsi_view* view = &v->view;
    struct wlr_xdg_toplevel* toplevel = view->wlr_xdg_toplevel;
    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;
    view_set_maximized(view, requested->maximized);
}

static void
handle_request_fullscreen(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_fullscreen from wlr_xdg_toplevel");

    struct bsi_xdg_shell_view* v =
        wl_container_of(listener, v, listen.request_fullscreen);
    struct bsi_view* view = &v->view;
    struct wlr_xdg_toplevel* toplevel = view->wlr_xdg_toplevel;
    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;
    view_set_fullscreen(view, requested->fullscreen);
}

static void
handle_request_minimize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_minimize from wlr_xdg_toplevel");

    struct bsi_xdg_shell_view* v =
        wl_container_of(listener, v, listen.request_minimize);
    struct bsi_view* view = &v->view;
    struct wlr_xdg_surface* surface = view->wlr_xdg_toplevel->base;
    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;
    view_set_minimized(view, requested->minimized);
}

static void
handle_request_move(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_move from wlr_xdg_toplevel");

    /* The user would like to begin an interactive move operation. This is
     * raised when a user clicks on the client side decorations. */
    struct bsi_xdg_shell_view* v =
        wl_container_of(listener, v, listen.request_move);
    struct bsi_view* view = &v->view;
    struct wlr_xdg_toplevel_move_event* event = data;
    union bsi_xdg_toplevel_event toplevel_event = { .move = event };

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        view_cursor_interactive(view, BSI_CURSOR_MOVE, toplevel_event);
    }
}

static void
handle_request_resize(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_resize from wlr_xdg_toplevel");

    /* The user would like to begin an interactive resize operation. This is
     * raised when a use clicks on the client side decorations. */
    struct bsi_xdg_shell_view* v =
        wl_container_of(listener, v, listen.request_resize);
    struct bsi_view* view = &v->view;
    struct wlr_xdg_toplevel_resize_event* event = data;
    union bsi_xdg_toplevel_event toplevel_event = { .resize = event };

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        view_cursor_interactive(view, BSI_CURSOR_RESIZE, toplevel_event);
    }
}

static void
handle_request_show_window_menu(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_show_window_menu from wlr_xdg_toplevel");

    struct bsi_xdg_shell_view* v =
        wl_container_of(listener, v, listen.request_show_window_menu);
    struct bsi_view* view = &v->view;
    struct wlr_xdg_toplevel_show_window_menu_event* event = data;
    union bsi_xdg_toplevel_event toplevel_event = { .show_window_menu = event };

    // TODO: Handle show window menu
    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        bsi_debug("Should show window menu");
    }
}

/* Global server handlers. */
void
handle_xdg_shell_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from wlr_xdg_shell");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.xdg_new_surface);
    struct wlr_xdg_surface* xdg_surface = data;

    assert(xdg_surface->role != WLR_XDG_SURFACE_ROLE_NONE);

    /* We must add xdg popups to the scene graph so they get rendered. The
     * wlroots scene graph provides a helper for this, but to use it we must
     * provide the proper parent scene node of the xdg popup. To enable this, we
     * always set the user data field of xdg_surfaces to the corresponding scene
     * node. */
    if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        if (wlr_surface_is_xdg_surface(xdg_surface->popup->parent)) {
            struct wlr_xdg_surface* parent_surface =
                wlr_xdg_surface_from_wlr_surface(xdg_surface->popup->parent);
            struct wlr_scene_node* parent_node = parent_surface->data;
            xdg_surface->data =
                wlr_scene_xdg_surface_create(parent_node, xdg_surface);
        } else if (wlr_surface_is_layer_surface(xdg_surface->popup->parent)) {
            struct wlr_layer_surface_v1* parent_surface =
                wlr_layer_surface_v1_from_wlr_surface(
                    xdg_surface->popup->parent);
            struct bsi_layer_surface_toplevel* parent_layer =
                parent_surface->data;
            struct wlr_scene_node* parent_node = parent_layer->scene_node->node;
            xdg_surface->data =
                wlr_scene_xdg_surface_create(parent_node, xdg_surface);
        }
    } else {
        struct bsi_output* output =
            wlr_output_layout_output_at(server->wlr_output_layout,
                                        server->wlr_cursor->x,
                                        server->wlr_cursor->y)
                ->data;
        struct bsi_workspace* workspace = workspaces_get_active(output);
        struct bsi_xdg_shell_view* view =
            calloc(1, sizeof(struct bsi_xdg_shell_view));

        view_init(&view->view, BSI_VIEW_TYPE_XDG_SHELL, &view_impl, server);
        view->view.wlr_xdg_toplevel = xdg_surface->toplevel;
        view->view.node =
            wlr_scene_xdg_surface_create(&server->wlr_scene->node, xdg_surface);
        view->view.node->data = &view->view;
        xdg_surface->toplevel->base->data = view->view.node;

        util_slot_connect(&xdg_surface->events.destroy,
                          &view->listen.destroy,
                          handle_destroy);
        util_slot_connect(
            &xdg_surface->events.map, &view->listen.map, handle_map);
        util_slot_connect(
            &xdg_surface->events.unmap, &view->listen.unmap, handle_unmap);

        util_slot_connect(&xdg_surface->toplevel->events.request_maximize,
                          &view->listen.request_maximize,
                          handle_request_maximize);
        util_slot_connect(&xdg_surface->toplevel->events.request_fullscreen,
                          &view->listen.request_fullscreen,
                          handle_request_fullscreen);
        util_slot_connect(&xdg_surface->toplevel->events.request_minimize,
                          &view->listen.request_minimize,
                          handle_request_minimize);
        util_slot_connect(&xdg_surface->toplevel->events.request_move,
                          &view->listen.request_move,
                          handle_request_move);
        util_slot_connect(&xdg_surface->toplevel->events.request_resize,
                          &view->listen.request_resize,
                          handle_request_resize);
        util_slot_connect(
            &xdg_surface->toplevel->events.request_show_window_menu,
            &view->listen.request_show_window_menu,
            handle_request_show_window_menu);

        /* Add wired up view to workspace on the active output. */
        workspace_view_add(workspace, &view->view);
        bsi_debug("Attached view to workspace %s", workspace->name);
        bsi_info("Workspace %s now has %d views",
                 workspace->name,
                 wl_list_length(&workspace->views));
    }
}

void
handle_xdg_request_activate(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_activate from wlr_xdg_activation");

    struct wlr_xdg_activation_v1_request_activate_event* event = data;

    if (!wlr_surface_is_xdg_surface(event->surface))
        return;

    struct wlr_xdg_surface* surface =
        wlr_xdg_surface_from_wlr_surface(event->surface);

    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    struct wlr_scene_node* node = surface->toplevel->base->data;
    struct bsi_view* view = node->data;

    if (view == NULL || !surface->mapped)
        return;

    view_request_activate(view);
}
