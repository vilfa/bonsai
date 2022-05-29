#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/util/log.h>

#include "bonsai/desktop/layer.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 */
void
bsi_layer_surface_toplevel_map_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.map);
    struct bsi_output* output = layer_toplevel->output;
    wlr_surface_send_enter(layer_toplevel->layer_surface->surface,
                           output->wlr_output);
}

void
bsi_layer_surface_toplevel_unmap_notify(struct wl_listener* listener,
                                        void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.unmap);
    struct bsi_output* output = layer_toplevel->output;
    if (wlr_seat_pointer_surface_has_focus(
            output->server->wlr_seat, layer_toplevel->layer_surface->surface))
        wlr_seat_pointer_notify_clear_focus(
            layer_toplevel->output->server->wlr_seat);
}

void
bsi_layer_surface_toplevel_destroy_notify(struct wl_listener* listener,
                                          void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_toplevel");

    struct bsi_layer_surface_toplevel* layer_toplevel =
        wl_container_of(listener, layer_toplevel, listen.destroy);
    union bsi_layer_surface layer_surface = { .toplevel = layer_toplevel };
    bsi_layer_surface_finish(layer_surface, BSI_LAYER_SURFACE_TOPLEVEL);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_TOPLEVEL);
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

/* wlr_surface -> wlr_layer_surface::surface */

void
bsi_layer_surface_toplevel_wlr_surface_commit_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event commit from bsi_layer_surface_toplevel");
}

void
bsi_layer_surface_toplevel_wlr_surface_new_subsurface_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event new_subsurface from bsi_layer_surface_toplevel");
}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> wlr_xdg_popup::base */

void
bsi_layer_surface_popup_destroy_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event popup_destroy from bsi_layer_surface_popup");

    struct bsi_layer_surface_popup* layer_popup =
        wl_container_of(listener, layer_popup, listen.destroy);
    union bsi_layer_surface layer_surface = { .popup = layer_popup };
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

    // TODO: Maybe add a function for this.
    /* Unconstrain popup to its largest parent box. */
    struct bsi_layer_surface_popup* toplevel_popup = layer_popup;
    while (toplevel_popup->parent_type == BSI_LAYER_SURFACE_POPUP) {
        toplevel_popup = toplevel_popup->parent.popup;
    }

    struct bsi_layer_surface_toplevel* toplevel_parent =
        toplevel_popup->parent.toplevel;
    struct wlr_box toplevel_parent_box = {
        .x = -toplevel_parent->box.x,
        .y = -toplevel_parent->box.y,
        .width = toplevel_parent->output->wlr_output->width,
        .height = toplevel_parent->output->wlr_output->height,
    };
    wlr_xdg_popup_unconstrain_from_box(layer_popup->popup,
                                       &toplevel_parent_box);
}

void
bsi_layer_surface_popup_map_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_popup");
}

void
bsi_layer_surface_popup_unmap_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_popup");
}

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface */

void
bsi_layer_surface_subsurface_destroy_notify(struct wl_listener* listener,
                                            void* data)
{
    bsi_debug("Got event destroy from bsi_layer_surface_subsurface");

    struct bsi_layer_surface_subsurface* layer_subsurface =
        wl_container_of(listener, layer_subsurface, listen.destroy);
    union bsi_layer_surface layer_surface = { .subsurface = layer_subsurface };
    bsi_layer_surface_finish(layer_surface, BSI_LAYER_SURFACE_SUBSURFACE);
    bsi_layer_surface_destroy(layer_surface, BSI_LAYER_SURFACE_SUBSURFACE);
}

void
bsi_layer_surface_subsurface_map_notify(struct wl_listener* listener,
                                        void* data)
{
    bsi_debug("Got event map from bsi_layer_surface_subsurface");
}

void
bsi_layer_surface_subsurface_unmap_notify(struct wl_listener* listener,
                                          void* data)
{
    bsi_debug("Got event unmap from bsi_layer_surface_subsurface");
}
