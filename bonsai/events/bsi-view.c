/**
 * @file bsi-view.c
 * @brief Contains all event listeners for `bsi_view`.
 *
 *
 *
 */

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"
#include "bonsai/scene/view.h"
#include "bonsai/server.h"

void
bsi_view_destroy_xdg_surface_notify(struct wl_listener* listener,
                                    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got destroy event from wlr_xdg_surface");

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.destroy_xdg_surface);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;

    bsi_view_listener_unlink_all(bsi_view);
    bsi_views_free(bsi_views, bsi_view);
}

void
bsi_view_destroy_scene_node_notify(struct wl_listener* listener,
                                   __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got destroy event from wlr_scene_node");

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.destroy_scene_node);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;

    bsi_view_listener_unlink_all(bsi_view);
    bsi_views_free(bsi_views, bsi_view);
}

void
bsi_view_ping_timeout_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got ping_timeout event from wlr_xdg_surface");
#warning "Not implemented"
}

void
bsi_view_new_popup_notify(__attribute((unused)) struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got new_popup event from wlr_xdg_surface");
#warning "Not implemented"
}

void
bsi_view_map_notify(struct wl_listener* listener,
                    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got map event from wlr_xdg_surface");

    struct bsi_view* bsi_view = wl_container_of(listener, bsi_view, events.map);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;

    bsi_views_add(bsi_views, bsi_view);
    bsi_view_focus(bsi_view);
}

void
bsi_view_unmap_notify(struct wl_listener* listener,
                      __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got unmap event from wlr_xdg_surface");

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.unmap);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;

    bsi_views_remove(bsi_views, bsi_view);
}

void
bsi_view_configure_notify(__attribute__((unused)) struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got configure event from wlr_xdg_surface");
#warning "Not implemented"
}

void
bsi_view_ack_configure_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got ack_configure event from wlr_xdg_surface");
#warning "Not implemented"
}

void
bsi_view_request_maximize_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got request_maximize event from wlr_xdg_toplevel");
#warning "Not implemented"
}

void
bsi_view_request_fullscreen_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got request_fullscreen event from wlr_xdg_toplevel");
#warning "Not implemented"
}

void
bsi_view_request_minimize_notify(struct wl_listener* listener,
                                 __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got request_minimize event from wlr_xdg_toplevel");

    struct bsi_view* bsi_view =
        wl_container_of(listener, bsi_view, events.request_minimize);
    struct bsi_views* bsi_views = &bsi_view->bsi_server->bsi_views;

    bsi_views_remove(bsi_views, bsi_view);
}

void
bsi_view_request_move_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got request_move event from wlr_xdg_toplevel");

    // TODO: Check event serial to make sure client can't do this any time.

#warning "Not implemented"
}

void
bsi_view_request_resize_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got request_resize event from wlr_xdg_toplevel");

    // TODO: Check event serial to make sure client can't do this any time.

#warning "Not implemented"
}

void
bsi_view_request_show_window_menu_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG,
            "Got request_show_window_menu event from wlr_xdg_toplevel");
#warning "Not implemented"
}

void
bsi_view_set_parent_notify(__attribute__((unused)) struct wl_listener* listener,
                           __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got set_parent event from wlr_xdg_toplevel");
#warning "Not implemented"
}

void
bsi_view_set_title_notify(__attribute__((unused)) struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got set_title event from wlr_xdg_toplevel");
#warning "Not implemented"
}

void
bsi_view_set_app_id_notify(__attribute__((unused)) struct wl_listener* listener,
                           __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got set_app_id event from wlr_xdg_toplevel");
#warning "Not implemented"
}
