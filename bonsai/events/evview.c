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
#include "bonsai/server.h"

#define GIMME_ALL_VIEW_EVENTS

void
bsi_view_destroy_xdg_surface_notify(struct wl_listener* listener,
                                    __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_xdg_surface");
#endif

    struct bsi_view* view =
        wl_container_of(listener, view, listen.destroy_xdg_surface);
    struct bsi_server* server = view->bsi_server;
    struct bsi_workspace* wspace = view->bsi_workspace;

    bsi_scene_remove(server, view);
    bsi_workspace_view_remove(wspace, view);
    bsi_view_finish(view);
    bsi_view_destroy(view);

#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG,
            "Workspace %s now has %ld views",
            wspace->name,
            wspace->len_views);
#endif
}

void
bsi_view_destroy_scene_node_notify(struct wl_listener* listener,
                                   __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_scene_node");
#endif

    struct bsi_view* view =
        wl_container_of(listener, view, listen.destroy_scene_node);

    /* Should also work if e.g. you close a window on another workspace with
     * kill. */
    bsi_workspace_view_remove(view->bsi_workspace, view);
    bsi_view_finish(view);
    bsi_view_destroy(view);
}

void
bsi_view_new_popup_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event new_popup from wlr_xdg_surface");
#endif
}

void
bsi_view_map_notify(struct wl_listener* listener,
                    __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event map from wlr_xdg_surface");
#endif

    struct bsi_view* view = wl_container_of(listener, view, listen.map);
    struct bsi_server* server = view->bsi_server;
    struct wlr_xdg_surface* wlr_xdg_surface = view->wlr_xdg_surface;

    struct wlr_xdg_toplevel_requested* requested =
        &wlr_xdg_surface->toplevel->requested;

    /* Only honor one request of this type. A surface can't request to be
     * maximized and minimized at the same time. */
    if (requested->fullscreen) {
        // TODO: Should there be code handling fullscreen_output_destroy, we
        // already handle moving views in workspace code
        bsi_view_set_fullscreen(view, requested->fullscreen);
        wlr_xdg_toplevel_set_fullscreen(view->wlr_xdg_surface->toplevel,
                                        requested->fullscreen);
    } else if (requested->maximized) {
        bsi_view_set_maximized(view, requested->maximized);
        wlr_xdg_toplevel_set_maximized(view->wlr_xdg_surface->toplevel,
                                       requested->maximized);
    } else if (requested->minimized) {
        bsi_view_set_minimized(view, requested->minimized);
    }

    view->mapped = true;
    bsi_scene_add(server, view);
    bsi_view_focus(view);
}

void
bsi_view_unmap_notify(struct wl_listener* listener,
                      __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event unmap from wlr_xdg_surface");
#endif

    struct bsi_view* view = wl_container_of(listener, view, listen.unmap);
    struct bsi_server* server = view->bsi_server;

    view->mapped = false;
    bsi_scene_remove(server, view);
}

void
bsi_view_configure_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event configure from wlr_xdg_surface");
#endif
}

void
bsi_view_ack_configure_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event ack_configure from wlr_xdg_surface");
#endif
}

void
bsi_view_request_maximize_notify(struct wl_listener* listener,
                                 __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_maximize from wlr_xdg_toplevel");
#endif

    // TODO: This should probably take into account the panels and such stuff.
    // Also take a look at
    // https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/blob/master/unstable/wlr-layer-shell-unstable-v1.xml

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_maximize);
    struct wlr_xdg_surface* surface = view->wlr_xdg_surface;

    /* I'm not sure if this is necessary, only a toplevel surface should be
     * able to request maximize anyway; also, only toplevel surfaces have
     * views. */
    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;

    bsi_view_set_maximized(view, requested->maximized);
    /* This surface should now consider itself (un-)maximized */
    wlr_xdg_toplevel_set_maximized(surface->toplevel, requested->maximized);
}

void
bsi_view_request_fullscreen_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_fullscreen from wlr_xdg_toplevel");
#endif

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_fullscreen);
    /* Only a toplevel surface can request fullscreen. */
    struct wlr_xdg_toplevel* toplevel = view->wlr_xdg_surface->toplevel;
    struct wlr_xdg_toplevel_requested* requested = &toplevel->requested;

    bsi_view_set_fullscreen(view, requested->fullscreen);
    /* This surface should now consider itself (un-)fullscreen. */
    wlr_xdg_toplevel_set_fullscreen(toplevel, requested->fullscreen);
}

void
bsi_view_request_minimize_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_minimize from wlr_xdg_toplevel");
#endif

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_minimize);
    struct bsi_server* server = view->bsi_server;
    struct wlr_xdg_surface* surface = data;

    /* Again, not sure if this is necessary, only a toplevel surface should be
     * able to request minimize anyway. */
    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    struct wlr_xdg_toplevel_requested* requested =
        &surface->toplevel->requested;

    /* Remove surface from views. */
    bsi_view_set_minimized(view, requested->minimized);
    bsi_scene_remove(server, view);
}

void
bsi_view_request_move_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event request_move from wlr_xdg_toplevel");
#endif

    /* The user would like to begin an interactive move operation. This is
     * raised when a user clicks on the client side decorations. */
    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_move);
    struct wlr_xdg_toplevel_move_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        bsi_view_interactive_begin(view, BSI_CURSOR_MOVE, 0);
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
    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_resize);
    struct wlr_xdg_toplevel_resize_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat, event->serial)) {
        bsi_view_interactive_begin(view, BSI_CURSOR_RESIZE, event->edges);
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

    struct bsi_view* view =
        wl_container_of(listener, view, listen.request_show_window_menu);
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
}

void
bsi_view_set_title_notify(struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event set_title from wlr_xdg_toplevel");
#endif

    struct bsi_view* view = wl_container_of(listener, view, listen.set_title);
    struct wlr_xdg_surface* surface = view->wlr_xdg_surface;

    if (surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        bsi_view_set_app_title(view, surface->toplevel->title);

    wlr_log(WLR_INFO, "Set title: %s", view->app_title);
}

void
bsi_view_set_app_id_notify(struct wl_listener* listener,
                           __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_VIEW_EVENTS
    wlr_log(WLR_DEBUG, "Got event set_app_id from wlr_xdg_toplevel");
#endif

    struct bsi_view* view = wl_container_of(listener, view, listen.set_app_id);
    struct wlr_xdg_surface* surface = view->wlr_xdg_surface;

    if (surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        bsi_view_set_app_id(view, surface->toplevel->app_id);

    wlr_log(WLR_INFO, "Set app id: %s", view->app_id);
}
