/**
 * @file bsi-workspace.c
 * @brief Contains all event handlers for `bsi_workspace`.
 *
 *
 */
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/server.h"

#define GIMME_ALL_WORKSPACE_EVENTS

void
bsi_workspace_active_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_WORKSPACE_EVENTS
    wlr_log(WLR_DEBUG, "Got event active from bsi_workspace");
#endif

    struct bsi_view* view =
        wl_container_of(listener, view, listen.active_workspace);

    // TODO: Figure this out.

    // struct bsi_views* bsi_views = &bsi_view->bsi_server->scene.views;
    // struct bsi_workspace* bsi_workspace = data;
    // if (bsi_workspace->active) {
    //     wl_signal_emit(&bsi_view->wlr_xdg_surface->events.map,
    //                    bsi_view->wlr_xdg_surface);
    // } else {
    //     wl_signal_emit(&bsi_view->wlr_xdg_surface->events.unmap,
    //                    bsi_view->wlr_xdg_surface);
    // }
}
