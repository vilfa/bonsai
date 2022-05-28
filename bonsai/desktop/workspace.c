#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
bsi_workspaces_add(struct bsi_output* bsi_output,
                   struct bsi_workspace* bsi_workspace)
{
    assert(bsi_output);
    assert(bsi_workspace);

    if (bsi_output->wspace.len > 0) {
        struct bsi_workspace* workspace_active =
            bsi_workspaces_get_active(bsi_output);
        assert(workspace_active);
        bsi_workspace_set_active(workspace_active, false);
        bsi_workspace_set_active(bsi_workspace, true);
    }

    ++bsi_output->wspace.len;
    wl_list_insert(&bsi_output->wspace.workspaces, &bsi_workspace->link);

    /* To whom it may concern.. */
    bsi_util_slot_connect(&bsi_workspace->signal.active,
                          &bsi_output->server->listen.workspace_active,
                          bsi_workspace_active_notify);
    bsi_util_slot_connect(&bsi_workspace->signal.active,
                          &bsi_output->listen.workspace_active,
                          bsi_workspace_active_notify);

    bsi_workspace_set_active(bsi_workspace, true);
}

void
bsi_workspaces_remove(struct bsi_output* bsi_output,
                      struct bsi_workspace* bsi_workspace)
{
    assert(bsi_output);
    assert(bsi_workspace);

    /* Cannot remove the last workspace */
    if (bsi_output->wspace.len == 1)
        return;

    /* Get the workspace adjacent to this one. */
    struct wl_list* workspace_adj_link;
    for (size_t i = 0; i < bsi_output->wspace.len - 1; ++i) {
        workspace_adj_link = bsi_output->wspace.workspaces.next;
    }

    struct bsi_workspace* workspace_adj =
        wl_container_of(workspace_adj_link, workspace_adj, link);

    /* Move all the views to adjacent workspace. */
    struct bsi_view* view;
    struct bsi_view* tmp;
    wl_list_for_each_safe(view, tmp, &bsi_workspace->views, link_workspace)
    {
        // wl_list_remove(&view->link);
        bsi_workspace_view_move(bsi_workspace, workspace_adj, view);
    }

    /* Take care of the workspaces state. */
    --bsi_output->wspace.len;
    wl_list_remove(&bsi_workspace->link);
    bsi_workspace_set_active(bsi_workspace, false);
    bsi_workspace_set_active(workspace_adj, true);
}

struct bsi_workspace*
bsi_workspaces_get_active(struct bsi_output* bsi_output)
{
    assert(bsi_output);

    struct bsi_workspace* wspace;
    wl_list_for_each(wspace, &bsi_output->wspace.workspaces, link)
    {
        if (wspace->active)
            return wspace;
    }

    /* Should not happen. */
    return NULL;
}

struct bsi_workspace*
bsi_workspace_init(struct bsi_workspace* bsi_workspace,
                   struct bsi_server* bsi_server,
                   struct bsi_output* bsi_output,
                   const char* name)
{
    assert(bsi_workspace);
    assert(bsi_server);
    assert(bsi_output);
    assert(name);

    bsi_workspace->server = bsi_server;
    bsi_workspace->output = bsi_output;
    bsi_workspace->id = bsi_output->wspace.len + 1;
    bsi_workspace->name = strdup(name);
    bsi_workspace->active = false;

    bsi_workspace->len_views = 0;
    wl_list_init(&bsi_workspace->views);
    wl_signal_init(&bsi_workspace->signal.active);

    return bsi_workspace;
}

void
bsi_workspace_finish(struct bsi_workspace* bsi_workspace)
{
    assert(bsi_workspace);

    wl_list_remove(&bsi_workspace->server->listen.workspace_active.link);
    wl_list_remove(&bsi_workspace->output->listen.workspace_active.link);
}

void
bsi_workspace_destroy(struct bsi_workspace* bsi_workspace)
{
    assert(bsi_workspace);

    free(bsi_workspace->name);
    free(bsi_workspace);
}

size_t
bsi_workspace_get_global_id(struct bsi_workspace* bsi_workspace)
{
    assert(bsi_workspace);

    struct bsi_output* bsi_output = bsi_workspace->output;
    if (bsi_output->id > 0)
        return bsi_output->id * 10 + bsi_workspace->id;
    else
        return bsi_workspace->id;
}

void
bsi_workspace_set_active(struct bsi_workspace* bsi_workspace, bool active)
{
    assert(bsi_workspace);

    bsi_workspace->active = active;
    wl_signal_emit(&bsi_workspace->signal.active, bsi_workspace);
}

void
bsi_workspace_view_add(struct bsi_workspace* bsi_workspace,
                       struct bsi_view* bsi_view)
{
    assert(bsi_workspace);
    assert(bsi_view);

    ++bsi_workspace->len_views;
    wl_list_insert(&bsi_workspace->views, &bsi_view->link_workspace);
    bsi_view->parent_workspace = bsi_workspace;
}

void
bsi_workspace_view_remove(struct bsi_workspace* bsi_workspace,
                          struct bsi_view* bsi_view)
{
    assert(bsi_workspace);
    assert(bsi_view);

    --bsi_workspace->len_views;
    wl_list_remove(&bsi_view->link_workspace);
    bsi_view->parent_workspace = NULL;
}

void
bsi_workspace_view_move(struct bsi_workspace* bsi_workspace_from,
                        struct bsi_workspace* bsi_workspace_to,
                        struct bsi_view* bsi_view)
{
    assert(bsi_workspace_from);
    assert(bsi_workspace_to);
    assert(bsi_view);

    bsi_workspace_view_remove(bsi_workspace_from, bsi_view);
    bsi_workspace_view_add(bsi_workspace_to, bsi_view);
}
