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

#include "bonsai/desktop/cursor.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/server.h"

#define GIMME_ALL_VIEW_EVENTS

void
bsi_view_destroy_xdg_surface_notify(struct wl_listener* listener,
                                    __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_xdg_surface");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.destroy_xdg_surface);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;
    struct bsi_workspace* bsi_workspace = bsi_view->bsi_workspace;

    bsi_view_listener_unlink_all(bsi_view);
    bsi_views_remove(bsi_views, bsi_view);
    bsi_workspace_view_remove(bsi_workspace, bsi_view);

#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG,
            "Workspace %s now has %ld views",
            bsi_workspace->name,
            bsi_workspace->len_views);
#endif

    bsi_view_destroy(bsi_view);
}

void
bsi_view_destroy_scene_node_notify(struct wl_listener* listener,
                                   __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_scene_node");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.destroy_scene_node);

    bsi_view_listener_unlink_all(bsi_view);
    /* Should also work if e.g. you close a window on another workspace with
     * kill. */
    bsi_workspace_view_remove(bsi_view->bsi_workspace, bsi_view);
    bsi_view_destroy(bsi_view);
}

void
bsi_view_ping_timeout_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event ping_timeout from wlr_xdg_surface");
#endif

#warning "Not implemented"
}

void
bsi_view_new_popup_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event new_popup from wlr_xdg_surface");
#endif
#warning "Not implemented"
}

void
bsi_view_map_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event map from wlr_xdg_surface");
#endif

    struct bsi_view* bsi_view = wl_container_of(listener, bsi_view, events.map);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;
    struct wlr_xdg_surface* wlr_xdg_surface = data;

    if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        struct wlr_xdg_toplevel_requested* requested =
            &wlr_xdg_surface->toplevel->requested;
        // TODO: Implement maximize for surface
    }

    bsi_view->mapped = true;
    bsi_views_add(bsi_views, bsi_view);
    bsi_view_focus(bsi_view);
}

void
bsi_view_unmap_notify(struct wl_listener* listener,
                      __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event unmap from wlr_xdg_surface");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.unmap);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;

    bsi_view->mapped = false;
    bsi_views_remove(bsi_views, bsi_view);
}

void
bsi_view_configure_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event configure from wlr_xdg_surface");
#endif
#warning "Not implemented"
}

void
bsi_view_ack_configure_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event ack_configure from wlr_xdg_surface");
#endif
#warning "Not implemented"
}

void
bsi_view_request_maximize_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_maximize from wlr_xdg_toplevel");
#endif

    // TODO: This should probably take into account the panels and such stuff.
    // Also take a look at
    // https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/blob/master/unstable/wlr-layer-shell-unstable-v1.xml

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_maximize);
    struct wlr_xdg_surface* surface = data;
    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;

    bsi_view_set_maximized(bsi_view, requested->maximized);
    /* This surface should now consider itself maximized */
    wlr_xdg_toplevel_set_maximized(surface, requested->maximized);
}

void
bsi_view_request_fullscreen_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_fullscreen from wlr_xdg_toplevel");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_fullscreen);
    struct wlr_xdg_toplevel_set_fullscreen_event* event = data;

    bsi_view_set_fullscreen(bsi_view, event->fullscreen);
    /* This surface should now consider itself (un-)fullscreen. */
    wlr_xdg_toplevel_set_fullscreen(event->surface, event->fullscreen);
}

void
bsi_view_request_minimize_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_minimize from wlr_xdg_toplevel");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_minimize);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;
    struct wlr_xdg_surface* surface = data;
    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;

    // TODO: Is this right? I have no clue.

    bsi_view_set_minimized(bsi_view, requested->minimized);
    bsi_views_remove(bsi_views, bsi_view);
}

void
bsi_view_request_move_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_move from wlr_xdg_toplevel");
#endif

    /* The user would like to begin an interactive move operation. This is
     * raised when a user clicks on the client side decorations. */
    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_move);
    struct wlr_xdg_toplevel_move_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        bsi_view_interactive_begin(bsi_view, BSI_CURSOR_MOVE, 0);
    } else {
#ifdef GIMME_ALL_VIEW_EVENTS
        wlr_log(WLR_DEBUG, "Invalid request_move event serial, dropping");
#endif
    }
}

void
bsi_view_request_resize_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_resize from wlr_xdg_toplevel");
#endif

    /* The user would like to begin an interactive resize operation. This is
     * raised when a use clicks on the client side decorations. */
    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_resize);
    struct wlr_xdg_toplevel_resize_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        bsi_view_interactive_begin(bsi_view, BSI_CURSOR_RESIZE, event->edges);
    } else {
#ifdef GIMME_ALL_VIEW_EVENTS
        wlr_log(WLR_DEBUG, "Invalid request_resize event serial, dropping");
#endif
    }
}

void
bsi_view_request_show_window_menu_notify(struct wl_listener* listener,
                                         void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG,
            "Got event request_show_window_menu from wlr_xdg_toplevel");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_show_window_menu);
    struct wlr_xdg_toplevel_show_window_menu_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
// TODO: Handle show window menu
#ifdef GIMME_ALL_VIEW_EVENTS
        wlr_log(WLR_DEBUG, "Validated serial");
#endif
    } else {
#ifdef GIMME_ALL_VIEW_EVENTS
        wlr_log(WLR_DEBUG,
                "Invalid request_show_window_menu event serial, dropping");
#endif
    }

#warning "Not implemented"
}

void
bsi_view_set_parent_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event set_parent from wlr_xdg_toplevel");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.set_parent);
    struct wlr_xdg_surface* wlr_xdg_surface = data;

    // TODO: This causes a crash :(.
    wlr_xdg_toplevel_set_parent(bsi_view->wlr_xdg_surface, wlr_xdg_surface);

#warning "Not implemented"
}

void
bsi_view_set_title_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event set_title from wlr_xdg_toplevel");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.set_parent);
    struct wlr_xdg_surface* wlr_xdg_surface = data;

    if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        bsi_view_set_app_title(bsi_view, wlr_xdg_surface->toplevel->title);

    wlr_log(WLR_INFO, "Set title: %s", bsi_view->app_title);
}

void
bsi_view_set_app_id_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event set_app_id from wlr_xdg_toplevel");
#endif

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.set_app_id);
    struct wlr_xdg_surface* wlr_xdg_surface = data;

    if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        bsi_view_set_app_id(bsi_view, wlr_xdg_surface->toplevel->app_id);

    wlr_log(WLR_INFO, "Set app id: %s", bsi_view->app_id);
}
