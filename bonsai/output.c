#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_management_v1.h>

#include "bonsai/desktop/decoration.h"
#include "bonsai/desktop/layers.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"
#include "pixman.h"

void
bsi_outputs_add(struct bsi_server* server, struct bsi_output* output)
{
    wl_list_insert(&server->output.outputs, &output->link_server);
}

void
bsi_outputs_remove(struct bsi_output* output)
{
    wl_list_remove(&output->link_server);
}

struct bsi_output*
bsi_outputs_find(struct bsi_server* server, struct wlr_output* wlr_output)
{
    assert(wl_list_length(&server->output.outputs));

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        if (output->output == wlr_output)
            return output;
    }

    return NULL;
}

struct bsi_output*
bsi_output_init(struct bsi_output* output,
                struct bsi_server* server,
                struct wlr_output* wlr_output)
{
    output->id = wl_list_length(&server->output.outputs);
    output->server = server;
    output->output = wlr_output;
    output->new = true;
    wlr_output->data = output;
    /* Set the usable size of the output. */
    struct wlr_box box = { 0 };
    wlr_output_effective_resolution(wlr_output, &box.width, &box.height);
    bsi_output_set_usable_box(output, &box);
    /* Initialize damage. Initialize output configuration. */
    output->damage = wlr_output_damage_create(wlr_output);
    /* Initialize workspaces. */
    wl_list_init(&output->workspaces);
    /* Initialize layer shell. */
    for (size_t i = 0; i < 4; ++i) {
        wl_list_init(&output->layers[i]);
    }

    struct timespec now = bsi_util_timespec_get();
    output->last_frame = now;

    return output;
}

void
bsi_output_set_usable_box(struct bsi_output* output, struct wlr_box* box)
{
    bsi_debug("Set output usable box to [%d, %d, %d, %d]",
              box->x,
              box->y,
              box->width,
              box->height);
    output->usable.x = box->x;
    output->usable.y = box->y;
    output->usable.width = box->width;
    output->usable.height = box->height;
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
bsi_output_destroy(struct bsi_output* output)
{
    bsi_info("Destroying output %ld/%s", output->id, output->output->name);

    wl_list_remove(&output->listen.frame.link);
    wl_list_remove(&output->listen.destroy.link);

    struct bsi_server* server = output->server;
    if (wl_list_length(&server->output.outputs) > 0) {
        /* Move the views on all workspaces that belong to the destroyed output
         * to the active workspace of the active output, or the first workspace
         * of the first output.  */

        bsi_debug("Moving member views to next workspace");

        struct bsi_output* next_output =
            wlr_output_layout_output_at(server->wlr_output_layout,
                                        server->wlr_cursor->x,
                                        server->wlr_cursor->y)
                ->data;
        struct bsi_workspace* next_output_wspace =
            bsi_workspaces_get_active(next_output);

        struct wl_list* curr_output_wspaces = &output->workspaces;
        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(
            wspace, wspace_tmp, curr_output_wspaces, link_output)
        {
            struct bsi_view* view;
            wl_list_for_each(view, &wspace->views, link_server)
            {
                bsi_workspace_view_move(wspace, next_output_wspace, view);
            }
            bsi_workspace_destroy(wspace);
        }
    } else {
        /* Destroy everything, there are no more outputs. */

        bsi_debug("Last output, destroy everything");

        struct wl_list* curr_output_wspaces = &output->workspaces;

        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(
            wspace, wspace_tmp, curr_output_wspaces, link_output)
        {
            bsi_debug("Destroying %d workspaces for output %ld/%s",
                      wl_list_length(&output->workspaces),
                      output->id,
                      output->output->name);

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
              output->output->name);
    for (size_t i = 0; i < 4; i++) {
        if (!wl_list_empty(&output->layers[i])) {
            struct bsi_layer_surface_toplevel *toplevel, *toplevel_tmp;
            wl_list_for_each_safe(
                toplevel, toplevel_tmp, &output->layers[i], link_output)
            {
                union bsi_layer_surface surf = { .toplevel = toplevel };
                wlr_layer_surface_v1_destroy(toplevel->layer_surface);
                bsi_layer_surface_destroy(surf, BSI_LAYER_SURFACE_TOPLEVEL);
            }
        }
    }

    free(output);
}

/**
 * Handlers
 */

void
handle_output_frame(struct wl_listener* listener, void* data)
{
    struct bsi_output* output = wl_container_of(listener, output, listen.frame);
    struct wlr_scene* wlr_scene = output->server->wlr_scene;

    struct wlr_scene_output* wlr_scene_output =
        wlr_scene_get_scene_output(wlr_scene, output->output);
    wlr_scene_output_commit(wlr_scene_output);

    struct timespec now = bsi_util_timespec_get();
    wlr_scene_output_send_frame_done(wlr_scene_output, &now);
}

void
handle_output_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_output");

    struct bsi_output* output =
        wl_container_of(listener, output, listen.destroy);
    struct bsi_server* server = output->server;

    if (wl_list_length(&server->output.outputs) == 1) {
        bsi_info("Last output destroyed, shutting down");
        server->shutting_down = true;
    }

    wlr_output_layout_remove(server->wlr_output_layout, output->output);
    bsi_outputs_remove(output);
    bsi_output_destroy(output);

    if (wl_list_length(&server->output.outputs) == 0) {
        bsi_debug("Out of outputs, exiting");
        bsi_server_exit(server);
    }
}

void
handle_output_damage_frame(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event damage_frame from wlr_output_damage");

    struct bsi_output* output =
        wl_container_of(listener, output, listen.damage_frame);

    // TODO: How the fuck do you render stuff compositor side?

    // struct bsi_server* server = output->server;

    // wlr_renderer_begin(
    //     server->wlr_renderer, output->output->width, output->output->height);

    // bool needs_frame;
    // pixman_region32_t damage;
    // pixman_region32_init(&damage);
    // if (!wlr_output_damage_attach_render(output->damage, &needs_frame,
    // &damage))
    //     return;

    // if (needs_frame) {
    //     struct bsi_decoration* deco;
    //     wl_list_for_each(deco, &server->scene.decorations, link_server)
    //     {
    //         float matrix[9];
    //         struct wlr_texture* texture;
    //         struct wlr_fbox source_box;
    //         bsi_decoration_draw(deco, &texture, matrix, &source_box);
    //         wlr_render_texture_with_matrix(
    //             output->output->renderer, texture, matrix, 1.0f);
    //     }
    // } else {
    //     wlr_output_rollback(output->output);
    // }

    // pixman_region32_fini(&damage);
}
