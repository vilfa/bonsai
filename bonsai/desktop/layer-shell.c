#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/layer.h"

void
bsi_layers_add(struct bsi_output* bsi_output,
               struct bsi_layer_surface_toplevel* bsi_layer_surface_toplevel,
               enum zwlr_layer_shell_v1_layer at_layer)
{
    assert(bsi_output);
    assert(bsi_layer_surface_toplevel);

    wl_list_insert(&bsi_output->layer.layers[at_layer],
                   &bsi_layer_surface_toplevel->link);
}

struct bsi_layer_surface_toplevel*
bsi_layer_surface_toplevel_init(
    struct bsi_layer_surface_toplevel* layer_surface,
    struct wlr_layer_surface_v1* wlr_layer_surface)
{
    assert(layer_surface);

    layer_surface->wlr_layer_surface = wlr_layer_surface;
    wlr_layer_surface->data = layer_surface;
    wl_list_init(&layer_surface->subsurfaces);
    return layer_surface;
}

struct bsi_layer_surface_popup*
bsi_layer_surface_popup_init(struct bsi_layer_surface_popup* layer_surface,
                             struct wlr_xdg_popup* wlr_xdg_popup,
                             enum bsi_layer_surface_type parent_type,
                             union bsi_layer_surface parent)
{
    assert(layer_surface);

    layer_surface->wlr_xdg_popup = wlr_xdg_popup;
    layer_surface->parent_type = parent_type;
    layer_surface->parent = parent;
    return layer_surface;
}

struct bsi_layer_surface_subsurface*
bsi_layer_surface_subsurface_init(
    struct bsi_layer_surface_subsurface* layer_subsurface,
    struct wlr_subsurface* wlr_subsurface,
    struct bsi_layer_surface_toplevel* member_of)
{
    assert(layer_subsurface);

    layer_subsurface->wlr_subsurface = wlr_subsurface;
    layer_subsurface->member_of = member_of;
    return layer_subsurface;
}

void
bsi_layer_surface_finish(union bsi_layer_surface bsi_layer_surface,
                         enum bsi_layer_surface_type layer_surface_type)
{
    switch (layer_surface_type) {
        case BSI_LAYER_SURFACE_TOPLEVEL: {
            struct bsi_layer_surface_toplevel* layer_surface =
                bsi_layer_surface.toplevel;
            /* wlr_layer_surface_v1 */
            wl_list_remove(&layer_surface->listen.map.link);
            wl_list_remove(&layer_surface->listen.unmap.link);
            wl_list_remove(&layer_surface->listen.destroy.link);
            wl_list_remove(&layer_surface->listen.new_popup.link);
            /* wlr_surface -> wlr_layer_surface::surface */
            wl_list_remove(&layer_surface->listen.wlr_surface_commit.link);
            wl_list_remove(
                &layer_surface->listen.wlr_surface_new_subsurface.link);
            break;
        }
        case BSI_LAYER_SURFACE_POPUP: {
            struct bsi_layer_surface_popup* layer_popup =
                bsi_layer_surface.popup;
            /* wlr_xdg_surface -> wlr_xdg_popup::base */
            wl_list_remove(&layer_popup->listen.destroy.link);
            wl_list_remove(&layer_popup->listen.new_popup.link);
            wl_list_remove(&layer_popup->listen.map.link);
            wl_list_remove(&layer_popup->listen.unmap.link);
            break;
        }
        case BSI_LAYER_SURFACE_SUBSURFACE: {
            struct bsi_layer_surface_subsurface* layer_subsurface =
                bsi_layer_surface.subsurface;
            /* wlr_subsurface */
            wl_list_remove(&layer_subsurface->listen.destroy.link);
            wl_list_remove(&layer_subsurface->listen.map.link);
            wl_list_remove(&layer_subsurface->listen.unmap.link);
            break;
        }
    }
}

void
bsi_layer_surface_destroy(union bsi_layer_surface bsi_layer_surface,
                          enum bsi_layer_surface_type layer_surface_type)
{
    switch (layer_surface_type) {
        case BSI_LAYER_SURFACE_TOPLEVEL:
            free(bsi_layer_surface.toplevel);
            break;
        case BSI_LAYER_SURFACE_POPUP:
            free(bsi_layer_surface.popup);
            break;
        case BSI_LAYER_SURFACE_SUBSURFACE:
            free(bsi_layer_surface.subsurface);
            break;
    }
}
