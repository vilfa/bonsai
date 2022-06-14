#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend/drm.h>
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

struct bsi_output*
output_init(struct bsi_output* output,
            struct bsi_server* server,
            struct wlr_output* wlr_output)
{
    output->id = wl_list_length(&server->output.outputs);
    output->server = server;
    output->output = wlr_output;
    output->added = true;
    output->destroying = false;
    wlr_output->data = output;
    /* Set the usable size of the output. */
    struct wlr_box box = { 0 };
    wlr_output_effective_resolution(wlr_output, &box.width, &box.height);
    output_set_usable_box(output, &box);
    /* Initialize damage. Initialize output configuration. */
    output->damage = wlr_output_damage_create(wlr_output);
    /* Initialize workspaces. */
    wl_list_init(&output->workspaces);
    /* Initialize layer shell. */
    for (size_t i = 0; i < 4; ++i) {
        wl_list_init(&output->layers[i]);
    }
    /* Initialize workspace listeners. */
    wl_list_init(&output->listen.workspace);

    struct timespec now = util_timespec_get();
    output->last_frame = now;

    return output;
}

void
output_set_usable_box(struct bsi_output* output, struct wlr_box* box)
{
    debug("Set output usable box to [%d, %d, %d, %d]",
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
output_surface_damage(struct bsi_output* output,
                      struct wlr_surface* wlr_surface,
                      bool entire_output)
{
    if (entire_output) {
        wlr_output_damage_add_whole(output->damage);
    } else {
        struct wlr_box box;
        wlr_surface_get_extends(wlr_surface, &box);
        wlr_output_damage_add_box(output->damage, &box);
    }
}

struct bsi_workspace*
output_get_next_workspace(struct bsi_output* output)
{
    struct bsi_workspace* curr;
    wl_list_for_each(curr, &output->workspaces, link_output)
    {
        if (curr->id == output->active_workspace->id + 1)
            return curr;
    }
    return output->active_workspace;
}

struct bsi_workspace*
output_get_prev_workspace(struct bsi_output* output)
{
    struct bsi_workspace* curr;
    wl_list_for_each_reverse(curr, &output->workspaces, link_output)
    {
        if (curr->id == output->active_workspace->id - 1)
            return curr;
    }
    return output->active_workspace;
}

static void
output_layer_arrange(struct bsi_output* output,
                     struct wl_list* shell_layer,
                     struct wlr_box* usable_box,
                     bool exclusive)
{
    struct wlr_box full_box = { 0 }; /* Full output area. */
    wlr_output_effective_resolution(
        output->output, &full_box.width, &full_box.height);

    struct bsi_layer_surface_toplevel* layer_toplevel;
    wl_list_for_each(layer_toplevel, shell_layer, link_output)
    {
        const char* layer_level =
            (layer_toplevel->at_layer == ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND)
                ? "ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND"
            : (layer_toplevel->at_layer == ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM)
                ? "ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM"
            : (layer_toplevel->at_layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP)
                ? "ZWLR_LAYER_SHELL_V1_LAYER_TOP"
            : (layer_toplevel->at_layer == ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY)
                ? "ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY"
                : "";

        struct wlr_layer_surface_v1* layer_surf = layer_toplevel->layer_surface;
        struct wlr_layer_surface_v1_state* state = &layer_surf->current;

        debug("Arranging layer surface at level '%s', wants exclusive %d",
              layer_level,
              state->exclusive_zone > 0);

        /* Check if this surface wants the layer exclusively. */
        if (exclusive != (state->exclusive_zone > 0))
            continue;

        struct wlr_box maxbounds_box = { 0 };
        if (state->exclusive_zone == -1)
            maxbounds_box = full_box;
        else
            maxbounds_box = *usable_box;

        struct wlr_box wants_box = {
            .width = state->desired_width,
            .height = state->desired_height,
        };

        const uint32_t horiz = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                               ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        const uint32_t vert = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                              ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;

        /* Arrange horizontally. */
        if (wants_box.width == 0)
            wants_box.x = maxbounds_box.x;
        else if (state->anchor & horiz)
            wants_box.x = maxbounds_box.x +
                          ((maxbounds_box.width / 2) - (wants_box.width / 2));
        else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)
            wants_box.x = maxbounds_box.x;
        else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)
            wants_box.x =
                maxbounds_box.x + (maxbounds_box.width - wants_box.width);
        else
            wants_box.x = maxbounds_box.x +
                          ((maxbounds_box.width / 2) - (wants_box.width / 2));

        /* Arrange vertically. */
        if (wants_box.width == 0)
            wants_box.y = maxbounds_box.y;
        else if (state->anchor & vert)
            wants_box.y = maxbounds_box.y +
                          ((maxbounds_box.height / 2) - (wants_box.height / 2));
        else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)
            wants_box.y = maxbounds_box.y;
        else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)
            wants_box.y =
                maxbounds_box.y + (maxbounds_box.height - wants_box.height);
        else
            wants_box.y = maxbounds_box.y +
                          ((maxbounds_box.height / 2) - (wants_box.height / 2));

        /* Arrange margins horizontally. */
        if (wants_box.width == 0) {
            wants_box.x += state->margin.left;
            wants_box.width = maxbounds_box.width -
                              (state->margin.left + state->margin.right);
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) {
            wants_box.x += state->margin.left;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) {
            wants_box.x -= state->margin.right;
        }

        /* Arrange margins vertically. */
        if (wants_box.height == 0) {
            wants_box.y += state->margin.left;
            wants_box.height = maxbounds_box.height -
                               (state->margin.top + state->margin.bottom);
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) {
            wants_box.y += state->margin.top;
        } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
            wants_box.y -= state->margin.bottom;
        }

        assert(wants_box.width >= 0 && wants_box.height >= 0);
        layer_toplevel->box = wants_box;

        /* Setup the available area of an output, so views can not cover
         * surfaces above them, but only if these are top layers. */
        if (exclusive && !layer_toplevel->exclusive_configured &&
            layer_toplevel->at_layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
            struct wlr_box usable_box = output->usable;
            if (state->anchor & horiz) {
                usable_box.y +=
                    (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)
                        ? wants_box.height
                        : 0;
                usable_box.height -= wants_box.height;
            } else if (state->anchor & vert) {
                usable_box.x +=
                    (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)
                        ? wants_box.width
                        : 0;
                usable_box.width -= wants_box.width;
            }
            output_set_usable_box(output, &usable_box);
            layer_toplevel->exclusive_configured = true;
        }

        wlr_layer_surface_v1_configure(
            layer_surf, wants_box.width, wants_box.height);
        wlr_scene_layer_surface_v1_configure(
            layer_toplevel->scene_node, &maxbounds_box, &wants_box);
    }
}

void
output_layers_arrange(struct bsi_output* output)
{
    struct wlr_box usable_box = { 0 }; /* Output usable area. */
    wlr_output_effective_resolution(
        output->output, &usable_box.width, &usable_box.height);

    /* Firstly, arrange exclusive layers. */
    for (size_t i = 0; i < 4; ++i) {
        if (!wl_list_empty(&output->layers[i]))
            output_layer_arrange(output, &output->layers[i], &usable_box, true);
    }

    /* Secondly, arrange non-exclusive layers. */
    for (size_t i = 0; i < 4; ++i) {
        if (!wl_list_empty(&output->layers[i]))
            output_layer_arrange(
                output, &output->layers[i], &usable_box, false);
    }

    /* Arrange layer positioning in global layout coordintes of its output. */
    for (size_t i = 0; i < 4; ++i) {
        struct bsi_layer_surface_toplevel* toplevel;
        if (!wl_list_empty(&output->layers[i])) {
            wl_list_for_each(toplevel, &output->layers[i], link_output)
            {
                struct wlr_box layout_box;
                wlr_output_layout_get_box(output->server->wlr_output_layout,
                                          output->output,
                                          &layout_box);
                int32_t lx, ly;
                wlr_scene_node_coords(
                    &toplevel->scene_node->tree->node, &lx, &ly);
                wlr_scene_node_set_position(&toplevel->scene_node->tree->node,
                                            lx + layout_box.x,
                                            ly + layout_box.y);
            }
        }
    }

    /* Arrange layers under & above windows. */
    for (size_t i = 0; i < 4; ++i) {
        struct bsi_layer_surface_toplevel* toplevel;
        if (!wl_list_empty(&output->layers[i])) {
            wl_list_for_each(toplevel, &output->layers[i], link_output)
            {
                if (toplevel->at_layer < ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
                    /* These are background layers, arrange the views
                     * appropriately.*/
                    debug("Lower layer with namespace '%s' to bottom",
                          toplevel->layer_surface->namespace);
                    wlr_scene_node_lower_to_bottom(
                        &toplevel->scene_node->tree->node);
                } else if (toplevel->at_layer >
                           ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM) {
                    /* These are foreground layers, arrange the views
                     * appropriately. */
                    debug("Raise layer with namespace '%s' to top",
                          toplevel->layer_surface->namespace);
                    wlr_scene_node_raise_to_top(
                        &toplevel->scene_node->tree->node);
                }
            }
        }
    }

    /* Arrange fullscreen views. */
    struct bsi_view* view;
    wl_list_for_each(
        view, &output->server->scene.views_fullscreen, link_fullscreen)
    {
        wlr_scene_node_raise_to_top(&view->tree->node);
    }

    /* If locked lock node should always be topmost. */
    struct bsi_server* server = output->server;
    if (server->session.locked && server->session.lock) {
        wlr_scene_node_raise_to_top(&server->session.lock->tree->node);
        return;
    }

    /* Last, focus the topmost keyboard-interactive layer, if it exists. */
    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(server->wlr_seat);
    static enum zwlr_layer_shell_v1_layer focusable_layers[] = {
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP,
    };
    for (size_t i = 0; i < 2; ++i) {
        struct bsi_layer_surface_toplevel* toplevel;
        wl_list_for_each(
            toplevel, &output->layers[focusable_layers[i]], link_output)
        {
            if (toplevel->layer_surface->current.keyboard_interactive) {
                wlr_seat_keyboard_notify_enter(server->wlr_seat,
                                               toplevel->layer_surface->surface,
                                               keyboard->keycodes,
                                               keyboard->num_keycodes,
                                               &keyboard->modifiers);
                return;
            }
        }
    }
}

void
output_destroy(struct bsi_output* output)
{
    info("Destroying output %ld/%s", output->id, output->output->name);

    wl_list_remove(&output->listen.frame.link);
    wl_list_remove(&output->listen.destroy.link);

    struct bsi_server* server = output->server;
    if (wl_list_length(&server->output.outputs) > 0) {
        /* Move the views on all workspaces that belong to the destroyed output
         * to the active workspace of the active output, or the first workspace
         * of the first output.  */

        debug("Moving member views to next workspace");

        struct bsi_output* next_output =
            wlr_output_layout_output_at(server->wlr_output_layout,
                                        server->wlr_cursor->x,
                                        server->wlr_cursor->y)
                ->data;
        struct bsi_workspace* next_output_wspace =
            workspaces_get_active(next_output);

        struct wl_list* curr_output_wspaces = &output->workspaces;
        struct bsi_workspace *wspace, *wspace_tmp;
        wl_list_for_each_safe(
            wspace, wspace_tmp, curr_output_wspaces, link_output)
        {
            struct bsi_view* view;
            wl_list_for_each(view, &wspace->views, link_server)
            {
                workspace_view_move(wspace, next_output_wspace, view);
            }
            workspace_destroy(wspace);
        }
    } else {
        /* Destroy everything, there are no more outputs. */

        debug("Last output, destroy everything");
        debug("Destroying %d workspaces for output %ld/%s",
              wl_list_length(&output->workspaces),
              output->id,
              output->output->name);

        struct bsi_workspace *ws, *ws_tmp;
        wl_list_for_each_safe(ws, ws_tmp, &output->workspaces, link_output)
        {
            /* The views take care of themselves -- they receive an xdg_surface
             * destroy event. Note that a workspace might be freed before the
             * views, kind of depends on scheduling. */
            workspace_destroy(ws);
        }
    }

    /* Cleanup of layer shell surfaces is taken care of by toplevel layer
     * output_destroy listeners. */
    debug("Destroying layer surfaces for output %ld/%s",
          output->id,
          output->output->name);
    for (size_t i = 0; i < 4; i++) {
        if (!wl_list_empty(&output->layers[i])) {
            struct bsi_layer_surface_toplevel *toplevel, *toplevel_tmp;
            wl_list_for_each_safe(
                toplevel, toplevel_tmp, &output->layers[i], link_output)
            {
                union bsi_layer_surface surf = { .toplevel = toplevel };
                if (!server->session.shutting_down)
                    wlr_layer_surface_v1_destroy(toplevel->layer_surface);
                layer_surface_destroy(surf, BSI_LAYER_SURFACE_TOPLEVEL);
            }
        }
    }

    free(output);
}

/* Handlers */
static void
handle_frame(struct wl_listener* listener, void* data)
{
    struct bsi_output* output = wl_container_of(listener, output, listen.frame);
    struct wlr_scene* wlr_scene = output->server->wlr_scene;

    struct wlr_scene_output* wlr_scene_output =
        wlr_scene_get_scene_output(wlr_scene, output->output);
    wlr_scene_output_commit(wlr_scene_output);

    struct timespec now = util_timespec_get();
    wlr_scene_output_send_frame_done(wlr_scene_output, &now);
}

static void
handle_destroy(struct wl_listener* listener, void* data)
{
    debug("Got event destroy from wlr_output");

    struct bsi_output* output =
        wl_container_of(listener, output, listen.destroy);
    struct bsi_server* server = output->server;

    if (wl_list_length(&server->output.outputs) == 1) {
        info("Last output destroyed, shutting down");
        server->session.shutting_down = true;
    }

    // wlr_output_layout_remove(server->wlr_output_layout, output->output);
    output->destroying = true;
    outputs_remove(output);
    output_destroy(output);

    if (wl_list_length(&server->output.outputs) == 0) {
        debug("Out of outputs, exiting");
        server_destroy(server);
        exit(EXIT_SUCCESS);
    }
}

static void
handle_damage_frame(struct wl_listener* listener, void* data)
{
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

/* Global server handlers. */
void
handle_new_output(struct wl_listener* listener, void* data)
{
    debug("Got event new_output from wlr_backend");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_output);
    struct wlr_output* wlr_output = data;

    wlr_output_init_render(
        wlr_output, server->wlr_allocator, server->wlr_renderer);

    /* Allocate output. */
    struct bsi_output* output = calloc(1, sizeof(struct bsi_output));
    output->output = wlr_output;
    outputs_add(server, output);

    /* Configure output. */
    bool has_config = false;
    struct bsi_config_atom* atom;
    wl_list_for_each(atom, &server->config.config->atoms, link)
    {
        if (atom->type == BSI_CONFIG_ATOM_OUTPUT) {
            has_config = atom->impl->apply(atom, server);
        }
    }

    struct wlr_output_mode* preffered_mode =
        wlr_output_preferred_mode(wlr_output);
    /* Set preffered mode first, if output has one. */
    if (preffered_mode && !has_config) {
        info("Output has preffered mode, setting %dx%d@%d",
             preffered_mode->width,
             preffered_mode->height,
             preffered_mode->refresh);

        wlr_output_set_mode(wlr_output, preffered_mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            error("Failed to commit on output '%s'", wlr_output->name);
            return;
        }
    } else if (!wl_list_empty(&wlr_output->modes) && !has_config) {
        struct wlr_output_mode* mode =
            wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            error("Failed to commit on output '%s'", wlr_output->name);
            return;
        }
    }

    output_init(output, server, wlr_output);

    /* Attach a workspace to the output. */
    char workspace_name[25];
    struct bsi_workspace* workspace = calloc(1, sizeof(struct bsi_workspace));
    sprintf(workspace_name,
            "Workspace %d",
            wl_list_length(&output->workspaces) + 1);
    workspace_init(workspace, server, output, workspace_name);
    workspaces_add(output, workspace);

    info("Attached %s to output %s", workspace->name, output->output->name);

    util_slot_connect(
        &output->output->events.frame, &output->listen.frame, handle_frame);
    util_slot_connect(&output->output->events.destroy,
                      &output->listen.destroy,
                      handle_destroy);
    util_slot_connect(&output->damage->events.frame,
                      &output->listen.damage_frame,
                      handle_damage_frame);

    struct wlr_output_configuration_v1* config =
        wlr_output_configuration_v1_create();
    struct wlr_output_configuration_head_v1* config_head =
        wlr_output_configuration_head_v1_create(config, output->output);
    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);

    wlr_output_layout_add_auto(server->wlr_output_layout, wlr_output);

    /* This if is kinda useless. */
    if (output->added) {
        outputs_setup_extern(server);
        output->added = false;
    }
}

void
handle_output_layout_change(struct wl_listener* listener, void* data)
{
    debug("Got event change from wlr_output_layout");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_layout_change);

    if (server->session.shutting_down)
        return;

    struct wlr_output_configuration_v1* config =
        wlr_output_configuration_v1_create();

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        struct wlr_output_configuration_head_v1* config_head =
            wlr_output_configuration_head_v1_create(config, output->output);
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->output, &output_box);
        config_head->state.mode = output->output->current_mode;
        if (!wlr_box_empty(&output_box)) {
            config_head->state.x = output_box.x;
            config_head->state.y = output_box.y;
        }

        /* Reset the usable box. */
        struct wlr_box usable_box = { 0 };
        wlr_output_effective_resolution(
            output->output, &usable_box.width, &usable_box.height);
        output_set_usable_box(output, &usable_box);

        /* Reset the state of the layer shell layers, with regards to output box
         * exclusive configuration. */
        for (size_t i = 0; i < 4; ++i) {
            struct bsi_layer_surface_toplevel* toplevel;
            wl_list_for_each(toplevel, &output->layers[i], link_output)
            {
                toplevel->exclusive_configured = false;
            }
        }

        output_layers_arrange(output);
        output_surface_damage(output, NULL, true);
    }

    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);
}

void
handle_output_manager_apply(struct wl_listener* listener, void* data)
{
    debug("Got event apply from wlr_output_manager");
    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_manager_apply);
    struct wlr_output_configuration_v1* config = data;

    // TODO: Disable outputs that need disabling and enable outputs that need
    // enabling.

    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);
    wlr_output_configuration_v1_send_succeeded(config);
    wlr_output_configuration_v1_destroy(config);
}

void
handle_output_manager_test(struct wl_listener* listener, void* data)
{
    debug("Got event test from wlr_output_manager");
    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_manager_test);
    struct wlr_output_configuration_v1* config = data;

    // TODO: Finish this.

    bool ok = true;
    struct wlr_output_configuration_head_v1* config_head;
    wl_list_for_each(config_head, &config->heads, link)
    {
        struct wlr_output* output = config_head->state.output;
        debug("Testing output %s", output->name);

        if (!wl_list_empty(&output->modes)) {
            struct wlr_output_mode* preffered_mode =
                wlr_output_preferred_mode(output);
            wlr_output_set_mode(output, preffered_mode);
        }

        if (wlr_output_is_drm(output)) {
            enum wl_output_transform transf =
                wlr_drm_connector_get_panel_orientation(output);
            if (output->transform != transf)
                wlr_output_set_transform(output, transf);
        }

        ok &= wlr_output_test(output);
    }

    if (ok)
        wlr_output_configuration_v1_send_succeeded(config);
    else
        wlr_output_configuration_v1_send_failed(config);

    wlr_output_configuration_v1_destroy(config);
}
