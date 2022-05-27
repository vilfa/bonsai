#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"

#define GIMME_ALL_LAYER_SHELL_EVENTS

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 */
void
bsi_layer_surface_toplevel_map_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event map from bsi_layer_surface_toplevel");
#endif
}

void
bsi_layer_surface_toplevel_unmap_notify(struct wl_listener* listener,
                                        void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event unmap from bsi_layer_surface_toplevel");
#endif
}

void
bsi_layer_surface_toplevel_destroy_notify(struct wl_listener* listener,
                                          void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from bsi_layer_surface_toplevel");
#endif
}

void
bsi_layer_surface_toplevel_new_popup_notify(struct wl_listener* listener,
                                            void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event new_popup from bsi_layer_surface_toplevel");
#endif
}

/* wlr_surface -> wlr_layer_surface::surface */

void
bsi_layer_surface_toplevel_wlr_surface_commit_notify(
    struct wl_listener* listener,
    void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event commit from bsi_layer_surface_toplevel");
#endif
}

void
bsi_layer_surface_toplevel_wlr_surface_new_subsurface_notify(
    struct wl_listener* listener,
    void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG,
            "Got event new_subsurface from bsi_layer_surface_toplevel");
#endif
}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> wlr_xdg_popup::base */

void
bsi_layer_surface_popup_destroy_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event popup_destroy from bsi_layer_surface_popup");
#endif
}

void
bsi_layer_surface_popup_new_popup_notify(struct wl_listener* listener,
                                         void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event new_popup from bsi_layer_surface_popup");
#endif
}

void
bsi_layer_surface_popup_map_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event map from bsi_layer_surface_popup");
#endif
}

void
bsi_layer_surface_popup_unmap_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event unmap from bsi_layer_surface_popup");
#endif
}

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface */

void
bsi_layer_surface_subsurface_destroy_notify(struct wl_listener* listener,
                                            void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from bsi_layer_surface_subsurface");
#endif
}

void
bsi_layer_surface_subsurface_map_notify(struct wl_listener* listener,
                                        void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event map from bsi_layer_surface_subsurface");
#endif
}

void
bsi_layer_surface_subsurface_unmap_notify(struct wl_listener* listener,
                                          void* data)
{
#ifdef GIMME_ALL_LAYER_SHELL_EVENTS
    wlr_log(WLR_DEBUG, "Got event unmap from bsi_layer_surface_subsurface");
#endif
}
