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

void
workspaces_add(struct bsi_output* output, struct bsi_workspace* workspace)
{
    if (wl_list_length(&output->workspaces) > 0) {
        struct bsi_workspace* workspace_active = workspaces_get_active(output);
        workspace_set_active(workspace_active, false);
        workspace_set_active(workspace, true);
    }

    wl_list_insert(&output->workspaces, &workspace->link_output);

    wl_list_insert(&output->server->listen.workspace,
                   &workspace->foreign_listeners[0].link); // bsi_server
    wl_list_insert(&output->listen.workspace,
                   &workspace->foreign_listeners[1].link); // bsi_output

    /* To whom it may concern... */
    util_slot_connect(&workspace->signal.active,
                      &workspace->foreign_listeners[0].active, // bsi_server
                      handle_server_workspace_active);
    util_slot_connect(&workspace->signal.active,
                      &workspace->foreign_listeners[1].active, // bsi_output
                      handle_output_workspace_active);

    workspace_set_active(workspace, true);
}

void
workspaces_remove(struct bsi_output* output, struct bsi_workspace* workspace)
{
    /* Cannot remove the last workspace */
    if (wl_list_length(&output->workspaces) == 1)
        return;

    /* Get the workspace adjacent to this one. */
    struct wl_list* workspace_adj_link;
    if (wl_list_length(&output->workspaces) > (int)workspace->id) {
        workspace_adj_link = output->workspaces.next;
    } else {
        workspace_adj_link = output->workspaces.prev;
    }

    struct bsi_workspace* workspace_adj =
        wl_container_of(workspace_adj_link, workspace_adj, link_output);

    /* Move all the views to adjacent workspace. */
    if (!wl_list_empty(&workspace->views)) {
        struct bsi_view *view, *view_tmp;
        wl_list_for_each_safe(view, view_tmp, &workspace->views, link_workspace)
        {
            workspace_view_move(workspace, workspace_adj, view);
        }
    }

    /* Take care of the workspaces state. */
    workspace_set_active(workspace, false);
    workspace_set_active(workspace_adj, true);
    wl_list_remove(&workspace->foreign_listeners[0].link); // bsi_server
    wl_list_remove(&workspace->foreign_listeners[1].link); // bsi_output
    util_slot_disconnect(&workspace->foreign_listeners[0].active);
    util_slot_disconnect(&workspace->foreign_listeners[1].active);
    wl_list_remove(&workspace->link_output);
}

struct bsi_workspace*
workspaces_get_active(struct bsi_output* output)
{
    struct bsi_workspace* workspace;
    wl_list_for_each(workspace, &output->workspaces, link_output)
    {
        if (workspace->active)
            return workspace;
    }

    /* Should not happen. */
    return NULL;
}

void
workspaces_next(struct bsi_output* output)
{
    bsi_info("Switch to next workspace");

    int32_t len_ws = wl_list_length(&output->workspaces);
    if (len_ws < (int32_t)output->server->config.workspaces_max &&
        (int32_t)output->active_workspace->id == len_ws - 1) {
        /* Attach a workspace to the output. */
        char workspace_name[25];
        struct bsi_workspace* workspace =
            calloc(1, sizeof(struct bsi_workspace));
        sprintf(workspace_name,
                "Workspace %d",
                wl_list_length(&output->workspaces) + 1);
        workspace_init(workspace, output->server, output, workspace_name);
        workspaces_add(output, workspace);

        bsi_info(
            "Created new workspace %ld/%s", workspace->id, workspace->name);
        bsi_info("Attached %ld/%s to output %ld/%s",
                 workspace->id,
                 workspace->name,
                 output->id,
                 output->output->name);
    } else {
        struct bsi_workspace* next_workspace =
            output_get_next_workspace(output);
        workspace_set_active(output->active_workspace, false);
        workspace_set_active(next_workspace, true);
    }
}

void
workspaces_prev(struct bsi_output* output)
{
    bsi_info("Switch to previous workspace");
    struct bsi_workspace* prev_workspace = output_get_prev_workspace(output);
    workspace_set_active(output->active_workspace, false);
    workspace_set_active(prev_workspace, true);
}

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
    view->parent_workspace = workspace;
}

void
workspace_view_remove(struct bsi_workspace* workspace, struct bsi_view* view)
{
    wl_list_remove(&view->link_workspace);
    util_slot_disconnect(&view->listen.workspace_active);
    view->parent_workspace = NULL;
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
        bsi_info("Show all views of workspace '%s'", workspace->name);

        wl_list_for_each(view, &workspace->views, link_workspace)
        {
            if (view->state == BSI_VIEW_STATE_MINIMIZED)
                view_set_minimized(view, false);
        }
    } else {
        bsi_info("Hide all views of workspace '%s'", workspace->name);

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

/**
 * Handlers
 */
void
handle_server_workspace_active(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event active for bsi_server from bsi_workspace");

    struct bsi_workspace* workspace = data;
    struct bsi_server* server = workspace->server;
    server->active_workspace = (workspace->active) ? workspace : NULL;
    bsi_info("Server workspace %ld/%s is now %s",
             workspace_get_global_id(workspace),
             workspace->name,
             (workspace->active) ? "active" : "inactive");
}

void
handle_output_workspace_active(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event active for bsi_output from bsi_workspace");

    struct bsi_workspace* workspace = data;
    struct bsi_output* output = workspace->output;
    output->active_workspace = (workspace->active) ? workspace : NULL;
    bsi_debug("Workspace %ld/%s for output %ld/%s is now %s",
              workspace_get_global_id(workspace),
              workspace->name,
              output->id,
              output->output->name,
              (workspace->active) ? "active" : "inactive");
}

void
handle_view_workspace_active(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event active for bsi_view from bsi_workspace");

    struct bsi_workspace* workspace = data;
    struct bsi_view* view =
        wl_container_of(listener, view, listen.workspace_active);
    wlr_scene_node_set_enabled(view->scene_node, workspace->active);
    bsi_debug("View with app_id '%s' of workspace %ld/%s is now %s",
              view->toplevel->app_id,
              workspace_get_global_id(workspace),
              workspace->name,
              (workspace->active) ? "enabled" : "disabled");
}
