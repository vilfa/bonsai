#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
bsi_workspaces_add(struct bsi_output* output, struct bsi_workspace* workspace)
{
    if (wl_list_length(&output->workspaces) > 0) {
        struct bsi_workspace* workspace_active =
            bsi_workspaces_get_active(output);
        bsi_workspace_set_active(workspace_active, false);
        bsi_workspace_set_active(workspace, true);
    }

    wl_list_insert(&output->workspaces, &workspace->link_output);

    /* To whom it may concern... */
    bsi_util_slot_connect(&workspace->signal.active,
                          &output->server->listen.workspace_active,
                          handle_server_workspace_active);
    bsi_util_slot_connect(&workspace->signal.active,
                          &output->listen.workspace_active,
                          handle_output_workspace_active);

    bsi_workspace_set_active(workspace, true);
}

void
bsi_workspaces_remove(struct bsi_output* output,
                      struct bsi_workspace* workspace)
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
            bsi_workspace_view_move(workspace, workspace_adj, view);
        }
    }

    /* Take care of the workspaces state. */
    wl_list_remove(&workspace->link_output);
    bsi_workspace_set_active(workspace, false);
    bsi_workspace_set_active(workspace_adj, true);
}

struct bsi_workspace*
bsi_workspaces_get_active(struct bsi_output* output)
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

struct bsi_workspace*
bsi_workspace_init(struct bsi_workspace* workspace,
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

    return workspace;
}

void
bsi_workspace_destroy(struct bsi_workspace* workspace)
{
    wl_list_remove(&workspace->server->listen.workspace_active.link);
    wl_list_remove(&workspace->output->listen.workspace_active.link);
    free(workspace->name);
    free(workspace);
}

size_t
bsi_workspace_get_global_id(struct bsi_workspace* workspace)
{
    struct bsi_output* bsi_output = workspace->output;
    if (bsi_output->id > 0)
        return bsi_output->id * 10 + workspace->id;
    else
        return workspace->id;
}

void
bsi_workspace_set_active(struct bsi_workspace* workspace, bool active)
{
    workspace->active = active;
    wl_signal_emit(&workspace->signal.active, workspace);
}

void
bsi_workspace_view_add(struct bsi_workspace* workspace, struct bsi_view* view)
{
    wl_list_insert(&workspace->views, &view->link_workspace);
    bsi_util_slot_connect(&workspace->signal.active,
                          &view->listen.workspace_active,
                          handle_view_workspace_active);
    view->parent_workspace = workspace;
}

void
bsi_workspace_view_remove(struct bsi_workspace* workspace,
                          struct bsi_view* view)
{
    wl_list_remove(&view->link_workspace);
    bsi_util_slot_disconnect(&view->listen.workspace_active);
    view->parent_workspace = NULL;
}

void
bsi_workspace_view_move(struct bsi_workspace* workspace_from,
                        struct bsi_workspace* workspace_to,
                        struct bsi_view* view)
{
    bsi_workspace_view_remove(workspace_from, view);
    bsi_workspace_view_add(workspace_to, view);
}

/**
 * Handlers
 */

void
handle_server_workspace_active(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event active for bsi_server from bsi_workspace");

    struct bsi_workspace* wspace = data;
    wspace->server->active_workspace = wspace;

    if (wspace->output->new) {
        bsi_output_setup_extern_progs(wspace->output);
        wspace->output->new = false;
    }

    bsi_debug("Active server workspace is now %ld/%s",
              bsi_workspace_get_global_id(wspace),
              wspace->name);
}

void
handle_output_workspace_active(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event active for bsi_output from bsi_workspace");

    struct bsi_workspace* wspace = data;
}

void
handle_view_workspace_active(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event active for bsi_view from bsi_workspace");

    struct bsi_workspace* wspace = data;
}
