/**
 * @file bsi-view.c
 * @brief Contains all event handlers for `bsi_view`.
 *
 *
 *
 */

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

struct bsi_view;

#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/input/cursor.h"
#include "bonsai/log.h"
#include "bonsai/server.h"

void
bsi_view_destroy_xdg_surface_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.destroy);
    struct bsi_server* server = view->server;
    struct bsi_workspace* workspace = view->parent_workspace;

    bsi_scene_remove_view(server, view);
    bsi_workspace_view_remove(workspace, view);
    bsi_view_finish(view);
    bsi_view_destroy(view);

    bsi_info("Workspace %s now has %ld views",
             workspace->name,
             workspace->len_views);
}

void
bsi_view_map_notify(struct wl_listener* listener, void* data)
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
            view->parent_workspace->output->wlr_output, &output_w, &output_h);
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
    bsi_scene_add_view(server, view);
    bsi_view_focus(view);
}

void
bsi_view_unmap_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from wlr_xdg_surface");

    struct bsi_view* view = wl_container_of(listener, view, listen.unmap);
    struct bsi_server* server = view->server;

    view->mapped = false;
    bsi_scene_remove_view(server, view);
}

void
bsi_view_request_maximize_notify(struct wl_listener* listener, void* data)
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
bsi_view_request_fullscreen_notify(struct wl_listener* listener, void* data)
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
bsi_view_request_minimize_notify(struct wl_listener* listener, void* data)
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
    bsi_scene_remove_view(server, view);
}

void
bsi_view_request_move_notify(struct wl_listener* listener, void* data)
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
bsi_view_request_resize_notify(struct wl_listener* listener, void* data)
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
bsi_view_request_show_window_menu_notify(struct wl_listener* listener,
                                         void* data)
{
    bsi_debug("Got event request_show_window_menu from wlr_xdg_toplevel");

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_show_window_menu);
    struct wlr_xdg_toplevel_show_window_menu_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial))
        bsi_debug("Should show window menu");
    // TODO: Handle show window menu
}
