#include <assert.h>
#include <math.h>
#include <stddef.h>
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

#include "bonsai/desktop/layers.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"
#include "pixman.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

void
bsi_layers_add(struct bsi_output* output,
               struct bsi_layer_surface_toplevel* layer)
{
    wl_list_insert(&output->layers[layer->at_layer], &layer->link_output);
}

void
bsi_layers_remove(struct bsi_layer_surface_toplevel* layer)
{
    wl_list_remove(&layer->link_output);
}

struct bsi_layer_surface_toplevel*
bsi_layer_surface_toplevel_init(struct bsi_layer_surface_toplevel* toplevel,
                                struct wlr_layer_surface_v1* layer_surface,
                                struct bsi_output* output)
{
    toplevel->layer_surface = layer_surface;
    toplevel->output = output;
    toplevel->mapped = false;
    toplevel->at_layer = layer_surface->pending.layer;
    layer_surface->data = toplevel;
    wl_list_init(&toplevel->subsurfaces);

    toplevel->scene_node = wlr_scene_layer_surface_v1_create(
        &output->server->wlr_scene->node, toplevel->layer_surface);
    toplevel->scene_node->node->data = toplevel;

    return toplevel;
}

struct bsi_layer_surface_popup*
bsi_layer_surface_popup_init(struct bsi_layer_surface_popup* popup,
                             struct wlr_xdg_popup* xdg_popup,
                             enum bsi_layer_surface_type parent_type,
                             union bsi_layer_surface parent)
{
    popup->popup = xdg_popup;
    popup->parent_type = parent_type;
    popup->parent = parent;
    return popup;
}

struct bsi_layer_surface_subsurface*
bsi_layer_surface_subsurface_init(
    struct bsi_layer_surface_subsurface* subsurface,
    struct wlr_subsurface* wlr_subsurface,
    struct bsi_layer_surface_toplevel* member_of)
{
    subsurface->subsurface = wlr_subsurface;
    subsurface->member_of = member_of;
    return subsurface;
}

struct bsi_layer_surface_toplevel*
bsi_layer_surface_get_toplevel_parent(union bsi_layer_surface layer_surface,
                                      enum bsi_layer_surface_type type)
{
    switch (type) {
        case BSI_LAYER_SURFACE_TOPLEVEL: {
            struct bsi_layer_surface_toplevel* toplevel =
                layer_surface.toplevel;
            return toplevel;
            break;
        }
        case BSI_LAYER_SURFACE_POPUP: {
            struct bsi_layer_surface_popup* popup = layer_surface.popup;
            while (popup->parent_type == BSI_LAYER_SURFACE_POPUP) {
                popup = popup->parent.popup;
            }
            return popup->parent.toplevel;
            break;
        }
        case BSI_LAYER_SURFACE_SUBSURFACE: {
            struct bsi_layer_surface_subsurface* subsurface =
                layer_surface.subsurface;
            return subsurface->member_of;
            break;
        }
    }
    return NULL;
}

void
bsi_layer_surface_focus(struct bsi_layer_surface_toplevel* toplevel)
{
    /* The `bsi_cursor_scene_data_at()` function always returns the topmost
     * scene node that any one surface belongs to, so we will always get a
     * toplevel layer shell surface as an argument for focus. */
    struct bsi_server* server = toplevel->output->server;
    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
    struct wlr_surface* prev_focused = seat->keyboard_state.focused_surface;

    /* The surface is already focused. */
    if (prev_focused == toplevel->layer_surface->surface)
        return;

    if (prev_focused && wlr_surface_is_xdg_surface(prev_focused)) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_focused);
        assert(prev_focused_xdg->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);
        wlr_xdg_toplevel_set_activated(prev_focused_xdg->toplevel, false);
    }

    wlr_seat_keyboard_notify_enter(seat,
                                   toplevel->layer_surface->surface,
                                   keyboard->keycodes,
                                   keyboard->num_keycodes,
                                   &keyboard->modifiers);
}

void
bsi_layer_surface_destroy(union bsi_layer_surface layer_surface,
                          enum bsi_layer_surface_type type)
{
    switch (type) {
        case BSI_LAYER_SURFACE_TOPLEVEL: {
            struct bsi_layer_surface_toplevel* toplevel =
                layer_surface.toplevel;
            /* wlr_layer_surface_v1 */
            wl_list_remove(&toplevel->listen.map.link);
            wl_list_remove(&toplevel->listen.unmap.link);
            wl_list_remove(&toplevel->listen.destroy.link);
            wl_list_remove(&toplevel->listen.new_popup.link);
            /* wlr_surface -> wlr_layer_surface::surface */
            wl_list_remove(&toplevel->listen.commit.link);
            wl_list_remove(&toplevel->listen.new_subsurface.link);
            if (!wl_list_empty(&toplevel->subsurfaces)) {
                struct bsi_layer_surface_subsurface *subsurf, *subsurf_tmp;
                wl_list_for_each_safe(
                    subsurf, subsurf_tmp, &toplevel->subsurfaces, link)
                {
                    wl_list_remove(&subsurf->link);
                    free(subsurf);
                }
            }
            free(toplevel);
            break;
        }
        case BSI_LAYER_SURFACE_POPUP: {
            struct bsi_layer_surface_popup* popup = layer_surface.popup;
            /* wlr_xdg_surface -> wlr_xdg_popup::base */
            wl_list_remove(&popup->listen.destroy.link);
            wl_list_remove(&popup->listen.new_popup.link);
            wl_list_remove(&popup->listen.map.link);
            wl_list_remove(&popup->listen.unmap.link);
            wl_list_remove(&popup->listen.commit.link);
            free(popup);
            break;
        }
        case BSI_LAYER_SURFACE_SUBSURFACE: {
            struct bsi_layer_surface_subsurface* subsurface =
                layer_surface.subsurface;
            /* wlr_subsurface */
            wl_list_remove(&subsurface->listen.destroy.link);
            wl_list_remove(&subsurface->listen.map.link);
            wl_list_remove(&subsurface->listen.unmap.link);
            free(subsurface);
            break;
        }
    }
}

static void
bsi_layer_arrange(struct bsi_output* output,
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
bsi_layers_output_arrange(struct bsi_output* output)
{
    struct wlr_box usable_box = { 0 }; /* Output usable area. */
    wlr_output_effective_resolution(
        output->output, &usable_box.width, &usable_box.height);

    /* Firstly, arrange exclusive layers. */
    for (size_t i = 0; i < 4; ++i) {
        if (!wl_list_empty(&output->layers[i]))
            bsi_layer_arrange(output, &output->layers[i], &usable_box, true);
    }

    /* Secondly, arrange non-exclusive layers. */
    for (size_t i = 0; i < 4; ++i) {
        if (!wl_list_empty(&output->layers[i]))
            bsi_layer_arrange(output, &output->layers[i], &usable_box, false);
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
                    bsi_debug("Lower layer with namespace '%s' to bottom",
                              toplevel->layer_surface->namespace);
                    wlr_scene_node_lower_to_bottom(toplevel->scene_node->node);
                } else if (toplevel->at_layer >
                           ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM) {
                    /* These are foreground layers, arrange the views
                     * appropriately. */
                    bsi_debug("Raise layer with namespace '%s' to top",
                              toplevel->layer_surface->namespace);
                    wlr_scene_node_raise_to_top(toplevel->scene_node->node);
                }
            }
        }
    }

    /* Last, focus the topmost keyboard-interactive layer, if it exists. */
    struct bsi_server* server = output->server;
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

/**
 * Handlers
 */
/**
 * On map and unmap events, the entire output surface should always be damaged.
 * On commit events, only the surface of the layer should be damaged.
 *
 * Kinda makes sense, we don't want to waste CPU time updating the surface of
 * the entire output, if a surface is already mapped, so only update that
 * surface area.
 *
 */
/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 -> layer_surface */
void
handle_layershell_toplvl_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.map);
    struct bsi_output* output = layer_toplevel->output;

    wlr_surface_send_enter(layer_toplevel->layer_surface->surface,
                           output->output);
    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_toplvl_unmap(struct wl_listener* listener, void* data)
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

    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_toplvl_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.destroy);
    struct bsi_output* output = layer_toplevel->output;
    struct bsi_server* server = output->server;

    if (server->shutting_down)
        return;

    /* Destroy the layer and rearrange this output. */
    union bsi_layer_surface layer_surface = { .toplevel = layer_toplevel };
    bsi_layers_remove(layer_toplevel);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_TOPLEVEL);
    bsi_layers_output_arrange(output);
    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_toplvl_new_popup(struct wl_listener* listener, void* data)
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
                          handle_layershell_popup_map);
    bsi_util_slot_connect(&xdg_popup->base->events.unmap,
                          &layer_popup->listen.unmap,
                          handle_layershell_popup_unmap);
    bsi_util_slot_connect(&xdg_popup->base->events.destroy,
                          &layer_popup->listen.destroy,
                          handle_layershell_popup_destroy);
    bsi_util_slot_connect(&xdg_popup->base->events.new_popup,
                          &layer_popup->listen.new_popup,
                          handle_layershell_popup_new_popup);
    bsi_util_slot_connect(&xdg_popup->base->surface->events.commit,
                          &layer_popup->listen.commit,
                          handle_layershell_popup_commit);

    /* Unconstrain popup to its largest parent box. */
    struct wlr_box toplevel_parent_box = {
        .x = -layer_toplevel->box.x,
        .y = -layer_toplevel->box.y,
        .width = layer_toplevel->output->output->width,
        .height = layer_toplevel->output->output->height,
    };
    wlr_xdg_popup_unconstrain_from_box(layer_popup->popup,
                                       &toplevel_parent_box);
}

/* wlr_surface -> layer_surface::surface */

void
handle_layershell_toplvl_commit(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.commit);
    struct bsi_output* output = layer_toplevel->output;

    bsi_debug("Toplevel namespace is '%s'",
              layer_toplevel->layer_surface->namespace);

    bool to_another_layer = false;
    if (layer_toplevel->layer_surface->current.committed != 0 ||
        layer_toplevel->mapped != layer_toplevel->layer_surface->mapped) {
        layer_toplevel->mapped = layer_toplevel->layer_surface->mapped;
        to_another_layer = layer_toplevel->at_layer !=
                           layer_toplevel->layer_surface->current.layer;
        if (to_another_layer) {
            // TODO: Maybe add a util function for moving a surface between
            // layers.
            wl_list_remove(&layer_toplevel->link_output);
            wl_list_insert(
                &output->layers[layer_toplevel->layer_surface->current.layer],
                &layer_toplevel->link_output);
        }
        bsi_layers_output_arrange(output);
    }

    bsi_output_surface_damage(
        layer_toplevel->output, layer_toplevel->layer_surface->surface, false);
}

void
handle_layershell_toplvl_new_subsurface(struct wl_listener* listener,
                                        void* data)
{
    bsi_debug("Got event new_subsurface from bsi_layer_surface_toplevel");

    struct wlr_subsurface* wlr_subsurface = data;
    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.new_subsurface);

    struct bsi_layer_surface_subsurface* layer_subsurface =
        calloc(1, sizeof(struct bsi_layer_surface_subsurface));
    bsi_layer_surface_subsurface_init(
        layer_subsurface, wlr_subsurface, layer_toplevel);
    wl_list_insert(&layer_toplevel->subsurfaces, &layer_subsurface->link);

    bsi_util_slot_connect(&wlr_subsurface->events.map,
                          &layer_subsurface->listen.map,
                          handle_layershell_subsurface_map);
    bsi_util_slot_connect(&wlr_subsurface->events.unmap,
                          &layer_subsurface->listen.unmap,
                          handle_layershell_subsurface_unmap);
    bsi_util_slot_connect(&wlr_subsurface->events.destroy,
                          &layer_subsurface->listen.destroy,
                          handle_layershell_subsurface_destroy);
    bsi_util_slot_connect(&wlr_subsurface->surface->events.commit,
                          &layer_subsurface->listen.commit,
                          handle_layershell_subsurface_commit);
}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> popup::base */

void
handle_layershell_popup_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.map);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;

    wlr_surface_send_enter(layer_popup->popup->base->surface, output->output);

    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_popup_unmap(struct wl_listener* listener, void* data)
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

    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_popup_destroy(struct wl_listener* listener, void* data)
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

    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_POPUP);
    bsi_output_surface_damage(toplevel_parent->output, NULL, true);
}

void
handle_layershell_popup_new_popup(struct wl_listener* listener, void* data)
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
                          handle_layershell_popup_map);
    bsi_util_slot_connect(&xdg_popup->base->events.unmap,
                          &layer_popup->listen.unmap,
                          handle_layershell_popup_unmap);
    bsi_util_slot_connect(&xdg_popup->base->events.destroy,
                          &layer_popup->listen.destroy,
                          handle_layershell_popup_destroy);
    bsi_util_slot_connect(&xdg_popup->base->events.new_popup,
                          &layer_popup->listen.new_popup,
                          handle_layershell_popup_new_popup);
    bsi_util_slot_connect(&xdg_popup->base->surface->events.commit,
                          &layer_popup->listen.commit,
                          handle_layershell_popup_commit);

    /* Unconstrain popup to its largest parent box. */
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct wlr_box toplevel_parent_box = {
        .x = -toplevel_parent->box.x,
        .y = -toplevel_parent->box.y,
        .width = toplevel_parent->output->output->width,
        .height = toplevel_parent->output->output->height,
    };

    wlr_xdg_popup_unconstrain_from_box(layer_popup->popup,
                                       &toplevel_parent_box);
}

/* wlr_xdg_surface -> popup::base::surface */
void
handle_layershell_popup_commit(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.commit);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        bsi_layer_surface_get_toplevel_parent(layer_surface,
                                              BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;

    bsi_output_surface_damage(output, layer_popup->popup->base->surface, false);
}

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface -> subsurface */

void
handle_layershell_subsurface_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.map);
    struct bsi_output* output = layer_subsurface->member_of->output;

    wlr_surface_send_enter(layer_subsurface->subsurface->surface,
                           output->output);
    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_subsurface_unmap(struct wl_listener* listener, void* data)
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

    bsi_output_surface_damage(output, NULL, true);
}

void
handle_layershell_subsurface_destroy(struct wl_listener* listener, void* data)
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

    wl_list_remove(&layer_subsurface->link);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_SUBSURFACE);
    bsi_output_surface_damage(toplevel_parent->output, NULL, true);
}

/* wlr_surface -> subsurface::surface */

void
handle_layershell_subsurface_commit(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.commit);
    struct bsi_output* output = layer_subsurface->member_of->output;

    bsi_output_surface_damage(
        output, layer_subsurface->subsurface->surface, false);
}
