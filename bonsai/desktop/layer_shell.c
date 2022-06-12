#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_region.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>

#include "bonsai/desktop/layers.h"
#include "bonsai/desktop/view.h"
#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"
#include "pixman.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include "xdg-shell-protocol.h"

struct bsi_layer_surface_toplevel*
layer_surface_toplevel_init(struct bsi_layer_surface_toplevel* toplevel,
                            struct wlr_layer_surface_v1* layer_surface,
                            struct bsi_output* output)
{
    toplevel->layer_surface = layer_surface;
    toplevel->output = output;
    toplevel->mapped = false;
    toplevel->at_layer = layer_surface->pending.layer;
    layer_surface->data = toplevel;
    wl_list_init(&toplevel->subsurfaces);
    toplevel->exclusive_configured = false;

    toplevel->scene_node = wlr_scene_layer_surface_v1_create(
        &output->server->wlr_scene->node, toplevel->layer_surface);
    toplevel->scene_node->node->data = toplevel;

    return toplevel;
}

struct bsi_layer_surface_popup*
layer_surface_popup_init(struct bsi_layer_surface_popup* popup,
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
layer_surface_subsurface_init(struct bsi_layer_surface_subsurface* subsurface,
                              struct wlr_subsurface* wlr_subsurface,
                              struct bsi_layer_surface_toplevel* member_of)
{
    subsurface->subsurface = wlr_subsurface;
    subsurface->member_of = member_of;
    return subsurface;
}

struct bsi_layer_surface_toplevel*
layer_surface_get_toplevel_parent(union bsi_layer_surface layer_surface,
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
layer_surface_focus(struct bsi_layer_surface_toplevel* toplevel)
{
    /* The `bsi_cursor_scene_data_at()` function always returns the topmost
     * scene node that any one surface belongs to, so we will always get a
     * toplevel layer shell surface as an argument for focus. */
    struct bsi_server* server = toplevel->output->server;
    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
    struct wlr_surface* prev_keyboard = seat->keyboard_state.focused_surface;

    /* The surface is already focused. */
    if (prev_keyboard == toplevel->layer_surface->surface)
        return;

    if (prev_keyboard && wlr_surface_is_xdg_surface(prev_keyboard)) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_keyboard);
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
layer_surface_destroy(union bsi_layer_surface layer_surface,
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

void
bsi_layer_move(struct bsi_layer_surface_toplevel* toplevel,
               struct bsi_output* output,
               enum zwlr_layer_shell_v1_layer to_layer)
{
    wl_list_remove(&toplevel->link_output);
    wl_list_insert(&output->layers[to_layer], &toplevel->link_output);
}

/* Handlers */
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
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface -> subsurface */
static void
handle_subsurface_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.map);
    struct bsi_output* output = layer_subsurface->member_of->output;

    wlr_surface_send_enter(layer_subsurface->subsurface->surface,
                           output->output);
    output_surface_damage(output, NULL, true);
}

static void
handle_subsurface_unmap(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.unmap);
    struct bsi_output* output = layer_subsurface->member_of->output;
    struct bsi_server* server = output->server;

    if (server->session.shutting_down)
        return;

    if (wlr_seat_pointer_surface_has_focus(
            output->server->wlr_seat, layer_subsurface->subsurface->surface))
        wlr_seat_pointer_notify_clear_focus(output->server->wlr_seat);

    output_surface_damage(output, NULL, true);
}

static void
handle_subsurface_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.destroy);
    union bsi_layer_surface layer_surface = { .subsurface = layer_subsurface };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        layer_surface_get_toplevel_parent(layer_surface,
                                          BSI_LAYER_SURFACE_TOPLEVEL);
    struct bsi_server* server = toplevel_parent->output->server;

    if (server->session.shutting_down)
        return;

    wl_list_remove(&layer_subsurface->link);
    layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_SUBSURFACE);
    output_surface_damage(toplevel_parent->output, NULL, true);
}

/* wlr_surface -> subsurface::surface */

static void
handle_subsurface_commit(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.commit);
    struct bsi_output* output = layer_subsurface->member_of->output;

    output_surface_damage(output, layer_subsurface->subsurface->surface, false);
}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> popup::base */
static void
handle_popup_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.map);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        layer_surface_get_toplevel_parent(layer_surface,
                                          BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;

    wlr_surface_send_enter(layer_popup->popup->base->surface, output->output);

    output_surface_damage(output, NULL, true);
}

static void
handle_popup_unmap(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.unmap);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        layer_surface_get_toplevel_parent(layer_surface,
                                          BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;
    struct bsi_server* server = output->server;

    if (server->session.shutting_down)
        return;

    if (wlr_seat_pointer_surface_has_focus(output->server->wlr_seat,
                                           layer_popup->popup->base->surface))
        wlr_seat_pointer_notify_clear_focus(output->server->wlr_seat);

    output_surface_damage(output, NULL, true);
}

static void
handle_popup_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event popup_destroy from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.destroy);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        layer_surface_get_toplevel_parent(layer_surface,
                                          BSI_LAYER_SURFACE_POPUP);
    struct bsi_server* server = toplevel_parent->output->server;

    if (server->session.shutting_down)
        return;

    layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_POPUP);
    output_surface_damage(toplevel_parent->output, NULL, true);
}

/* wlr_xdg_surface -> popup::base::surface */
static void
handle_popup_commit(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.commit);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        layer_surface_get_toplevel_parent(layer_surface,
                                          BSI_LAYER_SURFACE_POPUP);
    struct bsi_output* output = toplevel_parent->output;

    output_surface_damage(output, layer_popup->popup->base->surface, false);
}

/* wlr_xdg_surface -> popup::base */
static void
handle_popup_new_popup(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_popup from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* parent_popup =
        wl_container_of(listener, parent_popup, listen.new_popup);
    struct wlr_xdg_popup* xdg_popup = data;

    union bsi_layer_surface layer_parent = { .popup = parent_popup };
    struct bsi_layer_surface_popup* layer_popup =
        calloc(1, sizeof(struct bsi_layer_surface_popup));
    layer_surface_popup_init(
        layer_popup, xdg_popup, BSI_LAYER_SURFACE_POPUP, layer_parent);

    util_slot_connect(&xdg_popup->base->events.map,
                      &layer_popup->listen.map,
                      handle_popup_map);
    util_slot_connect(&xdg_popup->base->events.unmap,
                      &layer_popup->listen.unmap,
                      handle_popup_unmap);
    util_slot_connect(&xdg_popup->base->events.destroy,
                      &layer_popup->listen.destroy,
                      handle_popup_destroy);
    util_slot_connect(&xdg_popup->base->events.new_popup,
                      &layer_popup->listen.new_popup,
                      handle_popup_new_popup);
    util_slot_connect(&xdg_popup->base->surface->events.commit,
                      &layer_popup->listen.commit,
                      handle_popup_commit);

    /* Unconstrain popup to its largest parent box. */
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
    struct bsi_layer_surface_toplevel* toplevel_parent =
        layer_surface_get_toplevel_parent(layer_surface,
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

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 -> layer_surface */
static void
handle_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.map);
    struct bsi_output* output = layer_toplevel->output;

    wlr_surface_send_enter(layer_toplevel->layer_surface->surface,
                           output->output);
    output_surface_damage(output, NULL, true);
}

static void
handle_unmap(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.unmap);
    struct bsi_output* output = layer_toplevel->output;
    struct bsi_server* server = output->server;

    if (server->session.shutting_down)
        return;

    if (wlr_seat_pointer_surface_has_focus(
            output->server->wlr_seat, layer_toplevel->layer_surface->surface))
        wlr_seat_pointer_notify_clear_focus(
            layer_toplevel->output->server->wlr_seat);

    output_surface_damage(output, NULL, true);
}

static void
handle_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.destroy);
    struct bsi_output* output = layer_toplevel->output;
    struct bsi_server* server = output->server;

    if (server->session.shutting_down)
        return;

    /* Destroy the layer and rearrange this output. */
    union bsi_layer_surface layer_surface = { .toplevel = layer_toplevel };
    layers_remove(layer_toplevel);
    layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_TOPLEVEL);
    output_layers_arrange(output);
    output_surface_damage(output, NULL, true);
}

static void
handle_new_popup(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_popup from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.new_popup);
    struct wlr_xdg_popup* xdg_popup = data;

    union bsi_layer_surface layer_parent = { .toplevel = layer_toplevel };
    struct bsi_layer_surface_popup* layer_popup =
        calloc(1, sizeof(struct bsi_layer_surface_popup));
    layer_surface_popup_init(
        layer_popup, xdg_popup, BSI_LAYER_SURFACE_TOPLEVEL, layer_parent);

    util_slot_connect(&xdg_popup->base->events.map,
                      &layer_popup->listen.map,
                      handle_popup_map);
    util_slot_connect(&xdg_popup->base->events.unmap,
                      &layer_popup->listen.unmap,
                      handle_popup_unmap);
    util_slot_connect(&xdg_popup->base->events.destroy,
                      &layer_popup->listen.destroy,
                      handle_popup_destroy);
    util_slot_connect(&xdg_popup->base->events.new_popup,
                      &layer_popup->listen.new_popup,
                      handle_popup_new_popup);
    util_slot_connect(&xdg_popup->base->surface->events.commit,
                      &layer_popup->listen.commit,
                      handle_popup_commit);

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

static void
handle_commit(struct wl_listener* listener, void* data)
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
        if (to_another_layer)
            bsi_layer_move(layer_toplevel,
                           output,
                           layer_toplevel->layer_surface->current.layer);
        output_layers_arrange(output);
    }

    output_surface_damage(
        layer_toplevel->output, layer_toplevel->layer_surface->surface, false);
}

static void
handle_new_subsurface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_subsurface from bsi_layer_surface_toplevel");

    struct wlr_subsurface* wlr_subsurface = data;
    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.new_subsurface);

    struct bsi_layer_surface_subsurface* layer_subsurface =
        calloc(1, sizeof(struct bsi_layer_surface_subsurface));
    layer_surface_subsurface_init(
        layer_subsurface, wlr_subsurface, layer_toplevel);
    wl_list_insert(&layer_toplevel->subsurfaces, &layer_subsurface->link);

    util_slot_connect(&wlr_subsurface->events.map,
                      &layer_subsurface->listen.map,
                      handle_subsurface_map);
    util_slot_connect(&wlr_subsurface->events.unmap,
                      &layer_subsurface->listen.unmap,
                      handle_subsurface_unmap);
    util_slot_connect(&wlr_subsurface->events.destroy,
                      &layer_subsurface->listen.destroy,
                      handle_subsurface_destroy);
    util_slot_connect(&wlr_subsurface->surface->events.commit,
                      &layer_subsurface->listen.commit,
                      handle_subsurface_commit);
}

void
handle_layer_shell_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from wlr_layer_shell_v1");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.layer_new_surface);
    struct wlr_layer_surface_v1* layer_surface = data;

    struct bsi_output* active_output;
    if (!layer_surface->output) {
        active_output = wlr_output_layout_output_at(server->wlr_output_layout,
                                                    server->wlr_cursor->x,
                                                    server->wlr_cursor->y)
                            ->data;
        struct bsi_workspace* active_wspace =
            workspaces_get_active(active_output);
        layer_surface->output = active_wspace->output->output;
        layer_surface->output->data = active_output;
    } else {
        active_output = outputs_find(server, layer_surface->output);
        layer_surface->output->data = active_output;
    }

    /* Refuse this client, if a layer is already exclusively taken. */
    // if (!wl_list_empty(&active_output->layers[layer_surface->pending.layer]))
    // {
    //     struct bsi_layer_surface_toplevel* toplevel;
    //     wl_list_for_each(toplevel,
    //                      &active_output->layers[layer_surface->pending.layer],
    //                      link_output)
    //     {
    //         if (toplevel->layer_surface->current.exclusive_zone > 0) {
    //             bsi_debug(
    //                 "Refusing layer shell client wanting already exclusively
    //                 " "taken layer");
    //             bsi_debug("New client namespace '%s', exclusive client "
    //                       "namespace '%s'",
    //                       toplevel->layer_surface->namespace,
    //                       layer_surface->namespace);
    //             wlr_layer_surface_v1_destroy(layer_surface);
    //             return;
    //         }
    //     }
    // }

    struct bsi_layer_surface_toplevel* layer =
        calloc(1, sizeof(struct bsi_layer_surface_toplevel));
    layer_surface_toplevel_init(layer, layer_surface, active_output);
    util_slot_connect(
        &layer_surface->events.map, &layer->listen.map, handle_map);
    util_slot_connect(
        &layer_surface->events.unmap, &layer->listen.unmap, handle_unmap);
    util_slot_connect(
        &layer_surface->events.destroy, &layer->listen.destroy, handle_destroy);
    util_slot_connect(&layer_surface->events.new_popup,
                      &layer->listen.new_popup,
                      handle_new_popup);
    util_slot_connect(&layer_surface->surface->events.new_subsurface,
                      &layer->listen.new_subsurface,
                      handle_new_subsurface);
    util_slot_connect(&layer_surface->surface->events.commit,
                      &layer->listen.commit,
                      handle_commit);

    layers_add(active_output, layer);

    /* Overwrite the current state with pending, so we can look up the
     * desired state when arrangeing the surfaces. Then restore state for
     * wlr.*/
    struct wlr_layer_surface_v1_state old = layer_surface->current;
    layer_surface->current = layer_surface->pending;
    output_layers_arrange(active_output);
    layer_surface->current = old;
}
