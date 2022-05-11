#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/scene/workspace.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_workspaces*
bsi_workspaces_init(struct bsi_workspaces* bsi_workspaces)
{
    assert(bsi_workspaces);

    bsi_workspaces->len = 0;
    wl_list_init(&bsi_workspaces->workspaces);

    return bsi_workspaces;
}

void
bsi_workspaces_add(struct bsi_workspaces* bsi_workspaces,
                   struct bsi_workspace* bsi_workspace)
{
    assert(bsi_workspaces);
    assert(bsi_workspace);

    if (bsi_workspaces->len > 0) {
        struct bsi_workspace* workspace_active =
            bsi_workspaces_get_active(bsi_workspaces);
        assert(workspace_active);
        bsi_workspace_set_active(workspace_active, false);
        bsi_workspace_set_active(bsi_workspace, true);
    }

    ++bsi_workspaces->len;
    wl_list_insert(&bsi_workspaces->workspaces, &bsi_workspace->link);
    bsi_workspace_set_active(bsi_workspace, true);
}

void
bsi_workspaces_remove(struct bsi_workspaces* bsi_workspaces,
                      struct bsi_workspace* bsi_workspace)
{
    assert(bsi_workspaces);
    assert(bsi_workspace);

    /* Cannot remove the last workspace */
    if (bsi_workspaces->len == 1)
        return;

    /* Get the workspace adjacent to this one. */
    struct wl_list* workspace_adj_link;
    for (size_t i = 0; i < bsi_workspaces->len - 1; ++i) {
        workspace_adj_link = bsi_workspaces->workspaces.next;
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
    --bsi_workspaces->len;
    wl_list_remove(&bsi_workspace->link);
    bsi_workspace_set_active(bsi_workspace, false);
    bsi_workspace_set_active(workspace_adj, true);
}

struct bsi_workspace*
bsi_workspaces_get_active(struct bsi_workspaces* bsi_workspaces)
{
    assert(bsi_workspaces);

    struct bsi_workspace* workspace;
    wl_list_for_each(workspace, &bsi_workspaces->workspaces, link)
    {
        if (workspace->active)
            return workspace;
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

    bsi_workspace->bsi_server = bsi_server;
    bsi_workspace->bsi_output = bsi_output;
    bsi_workspace->id = bsi_server->bsi_workspaces.len + 1;
    bsi_workspace->name = strdup(name);
    bsi_workspace->active = false;

    bsi_workspace->len_views = 0;
    wl_list_init(&bsi_workspace->views);
    wl_signal_init(&bsi_workspace->events.active);

    return bsi_workspace;
}

void
bsi_workspace_destroy(struct bsi_workspace* bsi_workspace)
{
    assert(bsi_workspace);

    free(bsi_workspace->name);
    free(bsi_workspace);
}

void
bsi_workspace_set_active(struct bsi_workspace* bsi_workspace, bool active)
{
    assert(bsi_workspace);

    bsi_workspace->active = active;
    wl_signal_emit(&bsi_workspace->events.active, bsi_workspace);
}

void
bsi_workspace_view_add(struct bsi_workspace* bsi_workspace,
                       struct bsi_view* bsi_view)
{
    assert(bsi_workspace);
    assert(bsi_view);

    ++bsi_workspace->len_views;
    wl_list_insert(&bsi_workspace->views, &bsi_view->link_workspace);
}

void
bsi_workspace_view_remove(struct bsi_workspace* bsi_workspace,
                          struct bsi_view* bsi_view)
{
    assert(bsi_workspace);
    assert(bsi_view);

    --bsi_workspace->len_views;
    wl_list_remove(&bsi_view->link_workspace);
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
