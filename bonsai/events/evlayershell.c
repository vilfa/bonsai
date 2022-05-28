#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"
#include "bonsai/log.h"

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 */
void
bsi_layer_surface_toplevel_map_notify(struct wl_listener* listener, void* data)
{
    bsi_log(WLR_DEBUG, "Got event map from bsi_layer_surface_toplevel");
}

void
bsi_layer_surface_toplevel_unmap_notify(struct wl_listener* listener,
                                        void* data)
{
    bsi_log(WLR_DEBUG, "Got event unmap from bsi_layer_surface_toplevel");
}

void
bsi_layer_surface_toplevel_destroy_notify(struct wl_listener* listener,
                                          void* data)
{
    bsi_log(WLR_DEBUG, "Got event destroy from bsi_layer_surface_toplevel");
}

void
bsi_layer_surface_toplevel_new_popup_notify(struct wl_listener* listener,
                                            void* data)
{
    bsi_log(WLR_DEBUG, "Got event new_popup from bsi_layer_surface_toplevel");
}

/* wlr_surface -> wlr_layer_surface::surface */

void
bsi_layer_surface_toplevel_wlr_surface_commit_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_log(WLR_DEBUG, "Got event commit from bsi_layer_surface_toplevel");
}

void
bsi_layer_surface_toplevel_wlr_surface_new_subsurface_notify(
    struct wl_listener* listener,
    void* data)
{
    bsi_log(WLR_DEBUG,
            "Got event new_subsurface from bsi_layer_surface_toplevel");
}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> wlr_xdg_popup::base */

void
bsi_layer_surface_popup_destroy_notify(struct wl_listener* listener, void* data)
{
    bsi_log(WLR_DEBUG, "Got event popup_destroy from bsi_layer_surface_popup");
}

void
bsi_layer_surface_popup_new_popup_notify(struct wl_listener* listener,
                                         void* data)
{
    bsi_log(WLR_DEBUG, "Got event new_popup from bsi_layer_surface_popup");
}

void
bsi_layer_surface_popup_map_notify(struct wl_listener* listener, void* data)
{
    bsi_log(WLR_DEBUG, "Got event map from bsi_layer_surface_popup");
}

void
bsi_layer_surface_popup_unmap_notify(struct wl_listener* listener, void* data)
{
    bsi_log(WLR_DEBUG, "Got event unmap from bsi_layer_surface_popup");
}

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface */

void
bsi_layer_surface_subsurface_destroy_notify(struct wl_listener* listener,
                                            void* data)
{
    bsi_log(WLR_DEBUG, "Got event destroy from bsi_layer_surface_subsurface");
}

void
bsi_layer_surface_subsurface_map_notify(struct wl_listener* listener,
                                        void* data)
{
    bsi_log(WLR_DEBUG, "Got event map from bsi_layer_surface_subsurface");
}

void
bsi_layer_surface_subsurface_unmap_notify(struct wl_listener* listener,
                                          void* data)
{
    bsi_log(WLR_DEBUG, "Got event unmap from bsi_layer_surface_subsurface");
}
