#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_management_v1.h>

#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
bsi_outputs_add(struct bsi_server* server, struct bsi_output* output)
{
    ++server->output.len;
    wl_list_insert(&server->output.outputs, &output->link);
}

struct bsi_output*
bsi_outputs_find(struct bsi_server* server, struct wlr_output* wlr_output)
{
    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link)
    {
        if (output->wlr_output == wlr_output)
            return output;
    }

    /* Should not happen. */
    assert(NULL);
    return NULL;
}

void
bsi_outputs_remove(struct bsi_server* server, struct bsi_output* output)
{
    --server->output.len;
    wl_list_remove(&output->link);
}

struct bsi_output*
bsi_outputs_get_active(struct bsi_server* server)
{
    struct wlr_cursor* cursor = server->wlr_cursor;
    struct wlr_output* active_output = wlr_output_layout_output_at(
        server->wlr_output_layout, cursor->x, cursor->y);
    return active_output->data;
}

struct bsi_output*
bsi_output_init(struct bsi_output* output,
                struct bsi_server* server,
                struct wlr_output* wlr_output)
{
    output->id = server->output.len;
    output->server = server;
    output->wlr_output = wlr_output;
    output->new = true;
    /* Initialize damage. Initialize output configuration. */
    output->damage = wlr_output_damage_create(wlr_output);
    /* Initialize workspaces. */
    output->wspace.len = 0;
    wl_list_init(&output->wspace.workspaces);
    /* Initialize layer shell. */
    for (size_t i = 0; i < 4; ++i) {
        output->layer.len[i] = 0;
        wl_list_init(&output->layer.layers[i]);
    }

    struct timespec now = bsi_util_timespec_get();
    output->last_frame = now;

    return output;
}

void
bsi_output_setup_extern_progs(struct bsi_output* output)
{
    for (size_t i = 0; i < BSI_OUTPUT_EXTERN_PROG_MAX; ++i) {
        const char* exep = bsi_output_extern_progs[i];
        char* argsp = bsi_output_extern_progs_args[i];
        char** argp = NULL;
        size_t len_argp = bsi_util_split_argsp((char*)exep, argsp, " ", &argp);
        bsi_util_forkexec(argp, len_argp);
        bsi_util_split_free(&argp);
    }
}

void
bsi_output_surface_damage(struct bsi_output* output,
                          struct wlr_surface* wlr_surface,
                          bool entire_output)
{
    // TODO: This probably isn't right.

    if (entire_output) {
        wlr_output_damage_add_whole(output->damage);
    } else {
        struct wlr_box box;
        wlr_surface_get_extends(wlr_surface, &box);
        wlr_output_damage_add_box(output->damage, &box);

        // /* Get surface damage. */
        // pixman_region32_t damage;
        // pixman_region32_init(&damage);
        // wlr_surface_get_effective_damage(surface, &damage);
        // wlr_region_scale(&damage, &damage, output->wlr_output->scale);

        // /* If scaling has changed, expand damage region. */
        // if (ceil(output->wlr_output->scale) > surface->current.scale)
        //     wlr_region_expand(&damage,
        //                       &damage,
        //                       ceil(output->wlr_output->scale) -
        //                           surface->current.scale);

        // /* Translate the damage box to the output. */
        // pixman_region32_translate(&damage, box.x, box.y);
        // wlr_output_damage_add(output->damage, &damage);
        // pixman_region32_fini(&damage);

        // if (entire_output) {
        //     wlr_output_damage_add_box(output->damage, &box);
        // }

        // if (!wl_list_empty(&surface->current.frame_callback_list))
        //     wlr_output_schedule_frame(output->wlr_output);
    }
}

void
bsi_output_finish(struct bsi_output* output)
{
    wl_list_remove(&output->listen.frame.link);
    wl_list_remove(&output->listen.destroy.link);
}

void
bsi_output_destroy(struct bsi_output* output)
{
    bsi_info("Destroying output %ld/%s", output->id, output->wlr_output->name);

    struct bsi_server* server = output->server;
    if (server->output.len > 0) {
        /* Move the views on all workspaces that belong to the destroyed output
         * to the active workspace of the active output, or the first workspace
         * of the first output.  */

        bsi_debug("Moving member views to next workspace");

        struct bsi_output* next_output = bsi_outputs_get_active(server);
        struct bsi_workspace* next_output_wspace =
            bsi_workspaces_get_active(next_output);

        struct wl_list* curr_output_wspaces = &output->wspace.workspaces;
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

        struct wl_list* curr_output_wspaces = &output->wspace.workspaces;

        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(wspace, wspace_tmp, curr_output_wspaces, link)
        {
            bsi_debug("Destroying %ld workspaces for output %ld/%s",
                      output->wspace.len,
                      output->id,
                      output->wlr_output->name);

            /* The view take care of themselves -- they receive an xdg_surface
             * destroy event. Note that a workspace might be freed before the
             * views, kind of depends on scheduling. */

            bsi_workspace_destroy(wspace);
        }
    }

    /* Cleanup of layer shell surfaces is taken care of by toplevel layer
     * output_destroy listeners. */
    bsi_debug("Destroying layer surfaces for output %ld/%s",
              output->id,
              output->wlr_output->name);
    for (size_t i = 0; i < 4; i++) {
        if (!wl_list_empty(&output->layer.layers[i])) {
            struct bsi_layer_surface_toplevel *surf, *surf_tmp;
            wl_list_for_each_safe(
                surf, surf_tmp, &output->layer.layers[i], link)
            {
                union bsi_layer_surface lsurf = { .toplevel = surf };
                wlr_layer_surface_v1_destroy(surf->layer_surface);
                bsi_layer_surface_finish(lsurf, BSI_LAYER_SURFACE_TOPLEVEL);
                bsi_layer_surface_destroy(lsurf, BSI_LAYER_SURFACE_TOPLEVEL);
            }
        }
    }

    free(output);
}
