#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>

#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs, struct bsi_server* bsi_server)
{
    assert(bsi_outputs);
    assert(bsi_server);

    bsi_outputs->len = 0;
    bsi_outputs->bsi_server = bsi_server;
    wl_list_init(&bsi_outputs->outputs);

    return bsi_outputs;
}

void
bsi_outputs_add(struct bsi_outputs* bsi_outputs, struct bsi_output* bsi_output)
{
    assert(bsi_outputs);
    assert(bsi_output);

    ++bsi_outputs->len;
    wl_list_insert(&bsi_outputs->outputs, &bsi_output->link);
}

void
bsi_outputs_remove(struct bsi_outputs* bsi_outputs,
                   struct bsi_output* bsi_output)
{
    assert(bsi_outputs);
    assert(bsi_output);

    --bsi_outputs->len;
    wl_list_remove(&bsi_output->link);
}

struct bsi_output*
bsi_outputs_get_active(struct bsi_outputs* bsi_outputs)
{
    assert(bsi_outputs);

    struct bsi_server* bsi_server = bsi_outputs->bsi_server;
    struct wlr_cursor* cursor = bsi_server->wlr_cursor;

    struct wlr_output* active_output = wlr_output_layout_output_at(
        bsi_server->wlr_output_layout, cursor->x, cursor->y);

    struct bsi_output *first, *output;
    wl_list_for_each(output, &bsi_server->bsi_outputs.outputs, link)
    {
        if (first == NULL)
            first = output;

        if (output->wlr_output == active_output)
            return output;
    }

    assert(first);

    return first;
}

struct bsi_output*
bsi_output_init(struct bsi_output* bsi_output,
                struct bsi_server* bsi_server,
                struct wlr_output* wlr_output,
                struct bsi_workspaces* bsi_workspaces,
                struct bsi_output_layers* bsi_output_layers)
{
    assert(bsi_output);
    assert(bsi_server);
    assert(wlr_output);
    assert(bsi_workspaces);
    assert(bsi_output_layers);

    bsi_output->id = bsi_server->bsi_outputs.len;
    bsi_output->bsi_server = bsi_server;
    bsi_output->wlr_output = wlr_output;
    bsi_output->bsi_workspaces = bsi_workspaces;
    bsi_output->bsi_output_layers = bsi_output_layers;

    struct timespec now = bsi_util_timespec_get();
    bsi_output->last_frame = now;

    return bsi_output;
}

void
bsi_output_finish(struct bsi_output* bsi_output)
{
    assert(bsi_output);

    wl_list_remove(&bsi_output->listen.frame.link);
    wl_list_remove(&bsi_output->listen.damage.link);
    wl_list_remove(&bsi_output->listen.needs_frame.link);
    wl_list_remove(&bsi_output->listen.precommit.link);
    wl_list_remove(&bsi_output->listen.commit.link);
    wl_list_remove(&bsi_output->listen.present.link);
    wl_list_remove(&bsi_output->listen.bind.link);
    wl_list_remove(&bsi_output->listen.enable.link);
    wl_list_remove(&bsi_output->listen.mode.link);
    wl_list_remove(&bsi_output->listen.description.link);
    wl_list_remove(&bsi_output->listen.destroy.link);
}

void
bsi_output_destroy(struct bsi_output* bsi_output)
{
    assert(bsi_output);

    struct bsi_server* server = bsi_output->bsi_server;

    if (server->bsi_outputs.len > 0) {
        /* Move the views on all workspaces that belong to the destroyed output
         * to the active workspace of the active output, or the first workspace
         * of the first output.  */
        struct bsi_output* next_output =
            bsi_outputs_get_active(&server->bsi_outputs);
        struct bsi_workspace* next_workspace =
            bsi_workspaces_get_active(next_output->bsi_workspaces);

        struct bsi_workspaces* output_workspaces = bsi_output->bsi_workspaces;
        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(
            wspace, wspace_tmp, &output_workspaces->workspaces, link)
        {
            struct bsi_view* view;
            wl_list_for_each(view, &wspace->views, link)
            {
                bsi_workspace_view_move(wspace, next_workspace, view);
            }
            bsi_workspace_destroy(wspace);
        }
    } else {
        /* Destroy everything, there are no more outputs. */
        struct bsi_workspaces* output_workspaces = bsi_output->bsi_workspaces;

        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(
            wspace, wspace_tmp, &output_workspaces->workspaces, link)
        {
            struct bsi_view *view, *view_tmp;
            wl_list_for_each_safe(view, view_tmp, &wspace->views, link)
            {
                bsi_view_finish(view);
                bsi_view_destroy(view);
            }
            bsi_workspace_destroy(wspace);
        }
    }

    bsi_output_layers_destroy(bsi_output->bsi_output_layers);

    free(bsi_output);
}
