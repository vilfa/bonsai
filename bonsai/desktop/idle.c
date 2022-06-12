#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>

#include "bonsai/desktop/idle.h"
#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_idle_inhibitor*
idle_inhibitor_init(struct bsi_idle_inhibitor* inhibitor,
                    struct wlr_idle_inhibitor_v1* wlr_inhibitor,
                    struct bsi_server* server,
                    struct bsi_view* view,
                    enum bsi_idle_inhibit_mode mode)
{
    inhibitor->inhibitor = wlr_inhibitor;
    inhibitor->manager = server->wlr_idle_inhibit_manager;
    inhibitor->server = server;
    inhibitor->view = view;
    inhibitor->mode = mode;
    return inhibitor;
}

void
idle_inhibitor_destroy(struct bsi_idle_inhibitor* inhibitor)
{
    wl_list_remove(&inhibitor->listen.destroy.link);
    free(inhibitor);
}

bool
idle_inhibitor_active(struct bsi_idle_inhibitor* inhibitor)
{
    switch (inhibitor->mode) {
        case BSI_IDLE_INHIBIT_APPLICATION:
            /* Is acitve if view is visible. */
            assert(inhibitor->view);
            return inhibitor->view->state != BSI_VIEW_STATE_MINIMIZED &&
                   inhibitor->view->workspace ==
                       inhibitor->server->active_workspace &&
                   inhibitor->view->node->state.enabled;
        case BSI_IDLE_INHIBIT_USER:
            /* Inhibitor is destroyed manually by the user. */
            assert(!inhibitor->view);
            return true;
        case BSI_IDLE_INHIBIT_FULLSCREEN:
            assert(inhibitor->view);
            assert(!inhibitor->inhibitor);
            return inhibitor->view->state == BSI_VIEW_STATE_FULLSCREEN &&
                   inhibitor->view->workspace ==
                       inhibitor->server->active_workspace &&
                   inhibitor->view->node->state.enabled;
    }
    return false;
}

/* Handlers */
static void
handle_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_idle_inhibitor");

    struct bsi_idle_inhibitor* inhibitor =
        wl_container_of(listener, inhibitor, listen.destroy);
    struct bsi_server* server = inhibitor->server;
    idle_inhibitors_remove(inhibitor);
    idle_inhibitor_destroy(inhibitor);
    idle_inhibitors_update(server);
}

/* Global server handlers. */
void
handle_idle_manager_new_inhibitor(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_inhibitor from wlr_idle_inhibit_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_inhibitor);
    struct wlr_idle_inhibitor_v1* idle_inhibitor = data;

    /* Only xdg and layer shell surfaces can have inhbitors. */
    if (!wlr_surface_is_xdg_surface(idle_inhibitor->surface) &&
        !wlr_surface_is_layer_surface(idle_inhibitor->surface)) {
        bsi_info("Refuse to set inhbitor for non- xdg or layer shell surface");
        return;
    }

    struct bsi_idle_inhibitor* inhibitor =
        calloc(1, sizeof(struct bsi_idle_inhibitor));

    if (wlr_surface_is_xdg_surface(idle_inhibitor->surface)) {
        struct wlr_xdg_surface* xdg_surface =
            wlr_xdg_surface_from_wlr_surface(idle_inhibitor->surface);
        struct bsi_view* view = xdg_surface->data;
        idle_inhibitor_init(inhibitor,
                            idle_inhibitor,
                            server,
                            view,
                            BSI_IDLE_INHIBIT_APPLICATION);
    } else if (wlr_surface_is_layer_surface(idle_inhibitor->surface)) {
        idle_inhibitor_init(
            inhibitor, idle_inhibitor, server, NULL, BSI_IDLE_INHIBIT_USER);
    }

    util_slot_connect(&idle_inhibitor->events.destroy,
                      &inhibitor->listen.destroy,
                      handle_destroy);
    idle_inhibitors_add(server, inhibitor);

    idle_inhibitors_update(server);
}

void
handle_idle_activity_notify(struct wl_listener* listener, void* data)
{
    struct bsi_server* server =
        wl_container_of(listener, server, listen.activity_notify);
    idle_inhibitors_update(server);
}
