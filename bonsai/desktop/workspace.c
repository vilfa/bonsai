#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_workspace*
workspace_init(struct bsi_workspace* workspace,
               struct bsi_server* server,
               struct bsi_output* output,
               const char* name)
{
    workspace->server = server;
    workspace->output = output;
    workspace->id = wl_list_length(&output->workspaces);
    workspace->name = strdup(name);
    workspace->active = false;

    wl_list_init(&workspace->views);
    wl_signal_init(&workspace->signal.active);

    /* Better to explicitly have an array of size 2. */
    workspace->foreign_listeners =
        calloc(2, sizeof(struct bsi_workspace_listener));

    return workspace;
}

void
workspace_destroy(struct bsi_workspace* workspace)
{
    free(workspace->foreign_listeners);
    free(workspace->name);
    free(workspace);
}

size_t
workspace_get_global_id(struct bsi_workspace* workspace)
{
    struct bsi_output* bsi_output = workspace->output;
    if (bsi_output->id > 0)
        return bsi_output->id * 10 + workspace->id;
    else
        return workspace->id;
}

void
workspace_set_active(struct bsi_workspace* workspace, bool active)
{
    workspace->active = active;
    wl_signal_emit(&workspace->signal.active, workspace);
}

void
workspace_view_add(struct bsi_workspace* workspace, struct bsi_view* view)
{
    wl_list_insert(&workspace->views, &view->link_workspace);
    util_slot_connect(&workspace->signal.active,
                      &view->listen.workspace_active,
                      handle_view_workspace_active);
    view->workspace = workspace;
}

void
workspace_view_remove(struct bsi_workspace* workspace, struct bsi_view* view)
{
    wl_list_remove(&view->link_workspace);
    util_slot_disconnect(&view->listen.workspace_active);
    view->workspace = NULL;
}

void
workspace_view_move(struct bsi_workspace* workspace_from,
                    struct bsi_workspace* workspace_to,
                    struct bsi_view* view)
{
    workspace_view_remove(workspace_from, view);
    workspace_view_add(workspace_to, view);
}

void
workspace_views_show_all(struct bsi_workspace* workspace, bool show_all)
{
    if (wl_list_empty(&workspace->views))
        return;

    struct bsi_view* view;
    if (show_all) {
        info("Show all views of workspace '%s'", workspace->name);

        wl_list_for_each(view, &workspace->views, link_workspace)
        {
            if (view->state == BSI_VIEW_STATE_MINIMIZED)
                view_set_minimized(view, false);
        }
    } else {
        info("Hide all views of workspace '%s'", workspace->name);

        wl_list_for_each(view, &workspace->views, link_workspace)
        {
            switch (view->state) {
                case BSI_VIEW_STATE_MINIMIZED:
                    break;
                case BSI_VIEW_STATE_FULLSCREEN:
                    view_set_fullscreen(view, false);
                    view_set_minimized(view, true);
                    break;
                case BSI_VIEW_STATE_MAXIMIZED:
                    view_set_maximized(view, false);
                    view_set_minimized(view, true);
                    break;
                default:
                    view_set_minimized(view, true);
                    break;
            }
        }
    }
}

/* Handlers */
void
handle_server_workspace_active(struct wl_listener* listener, void* data)
{
    debug("Got event active for bsi_server from bsi_workspace");

    struct bsi_workspace* workspace = data;
    struct bsi_server* server = workspace->server;
    server->active_workspace = (workspace->active) ? workspace : NULL;
    info("Server workspace %ld/%s is now %s",
         workspace_get_global_id(workspace),
         workspace->name,
         (workspace->active) ? "active" : "inactive");
}

void
handle_output_workspace_active(struct wl_listener* listener, void* data)
{
    debug("Got event active for bsi_output from bsi_workspace");

    struct bsi_workspace* workspace = data;
    struct bsi_output* output = workspace->output;
    output->active_workspace = (workspace->active) ? workspace : NULL;
    debug("Workspace %ld/%s for output %ld/%s is now %s",
          workspace_get_global_id(workspace),
          workspace->name,
          output->id,
          output->output->name,
          (workspace->active) ? "active" : "inactive");
}

void
handle_view_workspace_active(struct wl_listener* listener, void* data)
{
    debug("Got event active for bsi_view from bsi_workspace");

    struct bsi_workspace* workspace = data;
    struct bsi_view* view =
        wl_container_of(listener, view, listen.workspace_active);
    wlr_scene_node_set_enabled(&view->tree->node, workspace->active);
    debug("View with app_id '%s' of workspace %ld/%s is now %s",
          view->wlr_xdg_toplevel->app_id,
          workspace_get_global_id(workspace),
          workspace->name,
          (workspace->active) ? "enabled" : "disabled");
}
