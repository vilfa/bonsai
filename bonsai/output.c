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

void
bsi_outputs_add(struct bsi_server* bsi_server, struct bsi_output* bsi_output)
{
    assert(bsi_server);
    assert(bsi_output);

    ++bsi_server->output.len;
    wl_list_insert(&bsi_server->output.outputs, &bsi_output->link);
}

void
bsi_outputs_remove(struct bsi_server* bsi_server, struct bsi_output* bsi_output)
{
    assert(bsi_server);
    assert(bsi_output);

    --bsi_server->output.len;
    wl_list_remove(&bsi_output->link);
}

struct bsi_output*
bsi_outputs_get_active(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    struct wlr_cursor* cursor = bsi_server->wlr_cursor;
    struct wlr_output* active_output = wlr_output_layout_output_at(
        bsi_server->wlr_output_layout, cursor->x, cursor->y);

    return active_output->data;
}

struct bsi_output*
bsi_output_init(struct bsi_output* bsi_output,
                struct bsi_server* bsi_server,
                struct wlr_output* wlr_output)
{
    assert(bsi_output);
    assert(bsi_server);
    assert(wlr_output);

    bsi_output->id = bsi_server->output.len;
    bsi_output->bsi_server = bsi_server;
    bsi_output->wlr_output = wlr_output;
    /* Initialize workspaces. */
    bsi_output->workspace.len = 0;
    wl_list_init(&bsi_output->workspace.workspaces);
    /* Initialize layer shell. */
    for (size_t i = 0; i < 4; ++i) {
        bsi_output->layer.len_layers[i] = 0;
        wl_list_init(&bsi_output->layer.layers[i]);
    }

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

    if (server->output.len > 0) {
        /* Move the views on all workspaces that belong to the destroyed output
         * to the active workspace of the active output, or the first workspace
         * of the first output.  */
        struct bsi_output* next_output = bsi_outputs_get_active(server);
        struct bsi_workspace* next_output_wspace =
            bsi_workspaces_get_active(next_output);

        struct wl_list* curr_output_wspaces = &bsi_output->workspace.workspaces;
        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(wspace, wspace_tmp, curr_output_wspaces, link)
        {
            struct bsi_view* view;
            wl_list_for_each(view, &wspace->views, link)
            {
                bsi_workspace_view_move(wspace, next_output_wspace, view);
            }
            bsi_workspace_destroy(wspace);
        }
    } else {
        /* Destroy everything, there are no more outputs. */
        struct wl_list* curr_output_wspaces = &bsi_output->workspace.workspaces;

        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(wspace, wspace_tmp, curr_output_wspaces, link)
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

    for (size_t i = 0; i < 4; ++i) {
        struct bsi_layer_surface_toplevel *surf, *surf_tmp;
        wl_list_for_each_safe(
            surf, surf_tmp, &bsi_output->layer.layers[i], link)
        {
            wlr_layer_surface_v1_destroy(surf->wlr_layer_surface);
            union bsi_layer_surface surface = { .toplevel = surf };
            bsi_layer_surface_finish(surface, BSI_LAYER_SURFACE_TOPLEVEL);
            bsi_layer_surface_destroy(surface, BSI_LAYER_SURFACE_TOPLEVEL);
        }
    }

    free(bsi_output);
}
