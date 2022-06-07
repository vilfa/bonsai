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

void
bsi_idle_inhibitors_add(struct bsi_server* server,
                        struct bsi_idle_inhibitor* inhibitor)
{
    wl_list_insert(&server->idle.inhibitors, &inhibitor->link_server);
}

void
bsi_idle_inhibitors_remove(struct bsi_idle_inhibitor* inhibitor)
{
    wl_list_remove(&inhibitor->link_server);
}

void
bsi_idle_inhibitors_state_update(struct bsi_server* server)
{
    if (wl_list_empty(&server->idle.inhibitors))
        return;

    bool inhibit = false;
    struct bsi_idle_inhibitor* idle;
    wl_list_for_each(idle, &server->idle.inhibitors, link_server)
    {
        if ((inhibit = bsi_idle_inhibitor_active(idle)))
            break;
    }

    wlr_idle_set_enabled(server->wlr_idle, NULL, !inhibit);
}

struct bsi_idle_inhibitor*
bsi_idle_inhibitor_init(struct bsi_idle_inhibitor* inhibitor,
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
bsi_idle_inhibitor_destroy(struct bsi_idle_inhibitor* inhibitor)
{
    wl_list_remove(&inhibitor->listen.destroy.link);
    free(inhibitor);
}

bool
bsi_idle_inhibitor_active(struct bsi_idle_inhibitor* inhibitor)
{
    switch (inhibitor->mode) {
        case BSI_IDLE_INHIBIT_APPLICATION:
            /* Is acitve if view is visible. */
            assert(inhibitor->view);
            return inhibitor->view->state != BSI_VIEW_STATE_MINIMIZED &&
                   inhibitor->view->parent_workspace ==
                       inhibitor->server->active_workspace &&
                   inhibitor->view->scene_node->state.enabled;
        case BSI_IDLE_INHIBIT_USER:
            /* Inhibitor is destroyed manually by the user. */
            assert(!inhibitor->view);
            return true;
        case BSI_IDLE_INHIBIT_FULLSCREEN:
            assert(inhibitor->view);
            assert(!inhibitor->inhibitor);
            return inhibitor->view->state == BSI_VIEW_STATE_FULLSCREEN &&
                   inhibitor->view->parent_workspace ==
                       inhibitor->server->active_workspace &&
                   inhibitor->view->scene_node->state.enabled;
    }
    return false;
}

/*
 * Handlers
 */
void
handle_idle_inhibitor_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_idle_inhibitor");

    struct bsi_idle_inhibitor* inhibitor =
        wl_container_of(listener, inhibitor, listen.destroy);
    struct bsi_server* server = inhibitor->server;
    bsi_idle_inhibitors_remove(inhibitor);
    bsi_idle_inhibitor_destroy(inhibitor);
    bsi_idle_inhibitors_state_update(server);
}
