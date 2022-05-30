#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_region.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>

#include "bonsai/desktop/layer.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"
#include "pixman.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

/**
 * On map and unmap events, the entire output surface should always be damaged.
 * On commit events, only the surface of the layer should be damaged.
 *
 * Kinda makes sense, we don't want to waste CPU time updating the surface of
 * the entire output, if a surface is already mapped, so only update that
 * surface area.
 *
 */

static void
bsi_output_layer_arrange(struct bsi_output* output,
                         struct wl_list* shell_layer,
                         struct wlr_box* usable_box,
                         bool exclusive)
{
    struct wlr_box full_box = { 0 }; /* Full output area. */
    wlr_output_effective_resolution(
        output->wlr_output, &full_box.width, &full_box.height);

    struct bsi_layer_surface_toplevel* layer_toplevel;
    wl_list_for_each(layer_toplevel, shell_layer, link)
    {
        struct wlr_layer_surface_v1* layer_surf = layer_toplevel->layer_surface;
        struct wlr_layer_surface_v1_state* state = &layer_surf->current;

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

        // TODO: Apply exclusive.

        wlr_layer_surface_v1_configure(
            layer_surf, wants_box.width, wants_box.height);
        wlr_scene_layer_surface_v1_configure(
            layer_toplevel->scene_node, &maxbounds_box, &wants_box);
    }
}

void
bsi_layers_arrange(struct bsi_output* output)
{
    struct wlr_box usable_box = { 0 }; /* Output usable area. */
    wlr_output_effective_resolution(
        output->wlr_output, &usable_box.width, &usable_box.height);

    /* Firstly, arrange exclusive layers. */
    for (size_t i = 0; i < 4; ++i) {
        if (output->layer.len[i] > 0) {
            bsi_output_layer_arrange(
                output, &output->layer.layers[i], &usable_box, true);
        }
    }

    /* Secondly, arrange non-exclusive layers. */
    for (size_t i = 0; i < 4; ++i) {
        if (output->layer.len[i] > 0) {
            bsi_output_layer_arrange(
                output, &output->layer.layers[i], &usable_box, false);
        }
    }

    /* Last, focus the topmost keyboard-interactive layer, if it exists. */
    // TODO: Focus topmost keyboard-interactive layer.
}

static void
bsi_output_surface_damage(struct bsi_output* output,
                          struct wlr_surface* surface,
                          bool entire_output)
{
    // TODO: This probably isn't right.

    wlr_output_damage_add_whole(output->damage);

    // struct wlr_box box;
    // wlr_output_layout_get_box(
    // output->server->wlr_output_layout, output->wlr_output, &box);

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

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 -> layer_surface */
void
bsi_layer_surface_toplevel_map_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.map);
    struct bsi_output* output = layer_toplevel->output;

    wlr_surface_send_enter(layer_toplevel->layer_surface->surface,
                           output->wlr_output);
    bsi_output_surface_damage(
        output, layer_toplevel->layer_surface->surface, true);
}

void
bsi_layer_surface_toplevel_unmap_notify(struct wl_listener* listener,
                                        void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.unmap);
    struct bsi_output* output = layer_toplevel->output;
    struct bsi_server* server = output->server;

    if (server->shutting_down)
        return;

    if (wlr_seat_pointer_surface_has_focus(
            output->server->wlr_seat, layer_toplevel->layer_surface->surface))
        wlr_seat_pointer_notify_clear_focus(
            layer_toplevel->output->server->wlr_seat);

    bsi_output_surface_damage(
        output, layer_toplevel->layer_surface->surface, true);
}

void
bsi_layer_surface_toplevel_destroy_notify(struct wl_listener* listener,
                                          void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.destroy);
    struct bsi_output* output = layer_toplevel->output;
    struct bsi_server* server = output->server;

    if (server->shutting_down)
        return;

    union bsi_layer_surface layer_surface = { .toplevel = layer_toplevel };
    bsi_layer_surface_finish(layer_surface, BSI_LAYER_SURFACE_TOPLEVEL);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_TOPLEVEL);
    /* Rearrange this output. */
    bsi_layers_arrange(output);
}

void
bsi_layer_surface_toplevel_new_popup_notify(struct wl_listener* listener,
                                            void* data)
{
    bsi_debug("Got event new_popup from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.new_popup);
    struct wlr_xdg_popup* xdg_popup = data;

    union bsi_layer_surface layer_parent = { .toplevel = layer_toplevel };
    struct bsi_layer_surface_popup* layer_popup =
        calloc(1, sizeof(struct bsi_layer_surface_popup));
    bsi_layer_surface_popup_init(
        layer_popup, xdg_popup, BSI_LAYER_SURFACE_TOPLEVEL, layer_parent);

    bsi_util_slot_connect(&xdg_popup->base->events.map,
                          &layer_popup->listen.map,
                          bsi_layer_surface_popup_map_notify);
    bsi_util_slot_connect(&xdg_popup->base->events.unmap,
                          &layer_popup->listen.unmap,
                          bsi_layer_surface_popup_unmap_notify);
    bsi_util_slot_connect(&xdg_popup->base->events.destroy,
                          &layer_popup->listen.destroy,
                          bsi_layer_surface_popup_destroy_notify);
    bsi_util_slot_connect(&xdg_popup->base->events.new_popup,
                          &layer_popup->listen.new_popup,
                          bsi_layer_surface_popup_new_popup_notify);
    bsi_util_slot_connect(&xdg_popup->base->surface->events.commit,
                          &layer_popup->listen.surface_commit,
                          bsi_layer_surface_popup_wlr_surface_commit_notify);

    // TODO: Maybe add a function for this.
    /* Unconstrain popup to its largest parent box. */
    struct wlr_box toplevel_parent_box = {
        .x = -layer_toplevel->box.x,
        .y = -layer_toplevel->box.y,
        .width = layer_toplevel->output->wlr_output->width,
        .height = layer_toplevel->output->wlr_output->height,
    };
    wlr_xdg_popup_unconstrain_from_box(layer_popup->popup,
                                       &toplevel_parent_box);
}

/* wlr_surface -> layer_surface::surface */

void
bsi_layer_surface_toplevel_wlr_surface_commit_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.surface_commit);
    struct bsi_output* output = layer_toplevel->output;

    bool to_another_layer = false;
    if (layer_toplevel->layer_surface->current.committed != 0 ||
        layer_toplevel->mapped != layer_toplevel->layer_surface->mapped) {
        layer_toplevel->mapped = layer_toplevel->layer_surface->mapped;
        to_another_layer = layer_toplevel->member_of_type !=
                           layer_toplevel->layer_surface->current.layer;
        if (to_another_layer) {
            // TODO: Maybe add a util function for moving a surface between
            // layers.
            wl_list_remove(&layer_toplevel->link);
            wl_list_insert(
                &output->layer
                     .layers[layer_toplevel->layer_surface->current.layer],
                &layer_toplevel->link);
        }
        bsi_layers_arrange(output);
    }

    // TODO: We should only be damaging the specific surface area, not the
    // entire output area here.
    bsi_output_surface_damage(
        layer_toplevel->output, layer_toplevel->layer_surface->surface, true);
}

void
bsi_layer_surface_toplevel_wlr_surface_new_subsurface_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event new_subsurface from bsi_layer_surface_toplevel");

    struct wlr_subsurface* wlr_subsurface = data;
    struct bsi_layer_surface_toplevel* layer_toplevel = wl_container_of(
        listener, layer_toplevel, listen.surface_new_subsurface);

    struct bsi_layer_surface_subsurface* layer_subsurface =
        calloc(1, sizeof(struct bsi_layer_surface_subsurface));
    bsi_layer_surface_subsurface_init(
        layer_subsurface, wlr_subsurface, layer_toplevel);
    wl_list_insert(&layer_toplevel->subsurfaces, &layer_subsurface->link);

    bsi_util_slot_connect(&wlr_subsurface->events.map,
                          &layer_subsurface->listen.map,
                          bsi_layer_surface_subsurface_map_notify);
    bsi_util_slot_connect(&wlr_subsurface->events.unmap,
                          &layer_subsurface->listen.unmap,
                          bsi_layer_surface_subsurface_unmap_notify);
    bsi_util_slot_connect(&wlr_subsurface->events.destroy,
                          &layer_subsurface->listen.destroy,
                          bsi_layer_surface_subsurface_destroy_notify);
    bsi_util_slot_connect(
        &wlr_subsurface->surface->events.commit,
        &layer_subsurface->listen.surface_commit,
        bsi_layer_surface_subsurface_wlr_surface_commit_notify);
}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> popup::base */

void
bsi_layer_surface_popup_map_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.map);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;

    wlr_surface_send_enter(layer_popup->popup->base->surface,
                           output->wlr_output);

    bsi_output_surface_damage(output, layer_popup->popup->base->surface, true);
}

void
bsi_layer_surface_popup_unmap_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.unmap);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;
    struct bsi_server* server = output->server;

    if (server->shutting_down)
        return;

    if (wlr_seat_pointer_surface_has_focus(output->server->wlr_seat,
                                           layer_popup->popup->base->surface))
        wlr_seat_pointer_notify_clear_focus(output->server->wlr_seat);

    bsi_output_surface_damage(output, layer_popup->popup->base->surface, true);
}

void
bsi_layer_surface_popup_destroy_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event popup_destroy from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.destroy);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct bsi_server* server = toplevel_parent->output->server;

    if (server->shutting_down)
        return;

    bsi_layer_surface_finish(layer_surface, BSI_LAYER_SURFACE_POPUP);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_POPUP);
}

void
bsi_layer_surface_popup_new_popup_notify(struct wl_listener* listener,
                                         void* data)
{
    bsi_debug("Got event new_popup from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* parent_popup =
        wl_container_of(listener, parent_popup, listen.new_popup);
    struct wlr_xdg_popup* xdg_popup = data;

    union bsi_layer_surface layer_parent = { .popup = parent_popup };
    struct bsi_layer_surface_popup* layer_popup =
        calloc(1, sizeof(struct bsi_layer_surface_popup));
    bsi_layer_surface_popup_init(
        layer_popup, xdg_popup, BSI_LAYER_SURFACE_POPUP, layer_parent);

    bsi_util_slot_connect(&xdg_popup->base->events.map,
                          &layer_popup->listen.map,
                          bsi_layer_surface_popup_map_notify);
    bsi_util_slot_connect(&xdg_popup->base->events.unmap,
                          &layer_popup->listen.unmap,
                          bsi_layer_surface_popup_unmap_notify);
    bsi_util_slot_connect(&xdg_popup->base->events.destroy,
                          &layer_popup->listen.destroy,
                          bsi_layer_surface_popup_destroy_notify);
    bsi_util_slot_connect(&xdg_popup->base->events.new_popup,
                          &layer_popup->listen.new_popup,
                          bsi_layer_surface_popup_new_popup_notify);
    bsi_util_slot_connect(&xdg_popup->base->surface->events.commit,
                          &layer_popup->listen.surface_commit,
                          bsi_layer_surface_popup_wlr_surface_commit_notify);

    /* Unconstrain popup to its largest parent box. */
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct wlr_box toplevel_parent_box = {
        .x = -toplevel_parent->box.x,
        .y = -toplevel_parent->box.y,
        .width = toplevel_parent->output->wlr_output->width,
        .height = toplevel_parent->output->wlr_output->height,
    };

    wlr_xdg_popup_unconstrain_from_box(layer_popup->popup,
                                       &toplevel_parent_box);
}

/* wlr_xdg_surface -> popup::base::surface */
void
bsi_layer_surface_popup_wlr_surface_commit_notify(struct wl_listener* listener,
                                                  void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.surface_commit);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;

    // TODO: We should only be damaging the specific surface area, not the
    // entire output area here.
    bsi_output_surface_damage(output, layer_popup->popup->base->surface, true);
}

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface -> subsurface */

void
bsi_layer_surface_subsurface_map_notify(struct wl_listener* listener,
                                        void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.map);
    struct bsi_output* output = layer_subsurface->member_of->output;

    wlr_surface_send_enter(layer_subsurface->subsurface->surface,
                           output->wlr_output);
    bsi_output_surface_damage(
        output, layer_subsurface->subsurface->surface, true);
}

void
bsi_layer_surface_subsurface_unmap_notify(struct wl_listener* listener,
                                          void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.unmap);
    struct bsi_output* output = layer_subsurface->member_of->output;
    struct bsi_server* server = output->server;

    if (server->shutting_down)
        return;

    if (wlr_seat_pointer_surface_has_focus(
            output->server->wlr_seat, layer_subsurface->subsurface->surface))
        wlr_seat_pointer_notify_clear_focus(output->server->wlr_seat);

    bsi_output_surface_damage(
        output, layer_subsurface->subsurface->surface, true);
}

void
bsi_layer_surface_subsurface_destroy_notify(struct wl_listener* listener,
                                            void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.destroy);
    union bsi_layer_surface layer_surface = { .subsurface = layer_subsurface };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_TOPLEVEL);
    struct bsi_server* server = toplevel_parent->output->server;

    if (server->shutting_down)
        return;

    bsi_layer_surface_finish(layer_surface, BSI_LAYER_SURFACE_SUBSURFACE);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_SUBSURFACE);
}

/* wlr_surface -> subsurface::surface */

void
bsi_layer_surface_subsurface_wlr_surface_commit_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.surface_commit);
    struct bsi_output* output = layer_subsurface->member_of->output;

    // TODO: We should only be damaging the specific surface area, not the
    // entire output area here.
    bsi_output_surface_damage(
        output, layer_subsurface->subsurface->surface, true);
}
