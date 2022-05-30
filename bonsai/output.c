#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>

#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/log.h"
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

struct bsi_output*
bsi_outputs_find(struct bsi_server* bsi_server, struct wlr_output* wlr_output)
{
    assert(bsi_server);
    assert(wlr_output);

    struct bsi_output* output;
    wl_list_for_each(output, &bsi_server->output.outputs, link)
    {
        if (output->wlr_output == wlr_output)
            return output;
    }

    /* Should not happen. */
    assert(false);
    return NULL;
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
    bsi_output->server = bsi_server;
    bsi_output->wlr_output = wlr_output;
    /* Initialize damage. */
    bsi_output->damage = wlr_output_damage_create(wlr_output);
    /* Initialize workspaces. */
    bsi_output->wspace.len = 0;
    wl_list_init(&bsi_output->wspace.workspaces);
    /* Initialize layer shell. */
    for (size_t i = 0; i < 4; ++i) {
        bsi_output->layer.len[i] = 0;
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
    wl_list_remove(&bsi_output->listen.destroy.link);
}

void
bsi_output_destroy(struct bsi_output* bsi_output)
{
    assert(bsi_output);

    bsi_info("Destroying output %ld/%s",
             bsi_output->id,
             bsi_output->wlr_output->name);

    struct bsi_server* server = bsi_output->server;
    if (server->output.len > 0) {
        /* Move the views on all workspaces that belong to the destroyed output
         * to the active workspace of the active output, or the first workspace
         * of the first output.  */

        bsi_debug("Moving member views to next workspace");

        struct bsi_output* next_output = bsi_outputs_get_active(server);
        struct bsi_workspace* next_output_wspace =
            bsi_workspaces_get_active(next_output);

        struct wl_list* curr_output_wspaces = &bsi_output->wspace.workspaces;
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

        bsi_debug("Last output, destroy everything");

        struct wl_list* curr_output_wspaces = &bsi_output->wspace.workspaces;

        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(wspace, wspace_tmp, curr_output_wspaces, link)
        {
            bsi_debug("Destroying %ld workspaces for output %ld/%s",
                      bsi_output->wspace.len,
                      bsi_output->id,
                      bsi_output->wlr_output->name);

            /* The view take care of themselves -- they receive an xdg_surface
             * destroy event. Note that a workspace might be freed before the
             * views, kind of depends on scheduling. */

            bsi_workspace_destroy(wspace);
        }
    }

    /* Cleanup of layer shell surfaces is taken care of by toplevel layer
     * output_destroy listeners. */
    bsi_debug("Destroying layer surfaces for output %ld/%s",
              bsi_output->id,
              bsi_output->wlr_output->name);
    for (size_t i = 0; i < 4; i++) {
        if (!wl_list_empty(&bsi_output->layer.layers[i])) {
            struct bsi_layer_surface_toplevel *surf, *surf_tmp;
            wl_list_for_each_safe(
                surf, surf_tmp, &bsi_output->layer.layers[i], link)
            {
                union bsi_layer_surface lsurf = { .toplevel = surf };
                wlr_layer_surface_v1_destroy(surf->layer_surface);
                bsi_layer_surface_finish(lsurf, BSI_LAYER_SURFACE_TOPLEVEL);
                bsi_layer_surface_destroy(lsurf, BSI_LAYER_SURFACE_TOPLEVEL);
            }
        }
    }

    free(bsi_output);
}
