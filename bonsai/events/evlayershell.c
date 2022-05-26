#include "bonsai/events.h"
#include <wayland-server-core.h>

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 */
void
bsi_layer_surface_toplevel_map_notify(struct wl_listener* listener, void* data)
{}

void
bsi_layer_surface_toplevel_unmap_notify(struct wl_listener* listener,
                                        void* data)
{}

void
bsi_layer_surface_toplevel_destroy_notify(struct wl_listener* listener,
                                          void* data)
{}

void
bsi_layer_surface_toplevel_new_popup_notify(struct wl_listener* listener,
                                            void* data)
{}

/* wlr_surface -> wlr_layer_surface::surface */

void
bsi_layer_surface_toplevel_wlr_surface_commit_notify(
    struct wl_listener* listener,
    void* data)
{}

void
bsi_layer_surface_toplevel_wlr_surface_new_subsurface_notify(
    struct wl_listener* listener,
    void* data)
{}

void
bsi_layer_surface_toplevel_wlr_surface_destroy_notify(
    struct wl_listener* listener,
    void* data)
{}

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> wlr_xdg_popup::base */

void
bsi_layer_surface_popup_destroy_notify(struct wl_listener* listener, void* data)
{}

void
bsi_layer_surface_popup_ping_timeout_notify(struct wl_listener* listener,
                                            void* data)
{}

void
bsi_layer_surface_popup_new_popup_notify(struct wl_listener* listener,
                                         void* data)
{}

void
bsi_layer_surface_popup_map_notify(struct wl_listener* listener, void* data)
{}

void
bsi_layer_surface_popup_unmap_notify(struct wl_listener* listener, void* data)
{}

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface */

void
bsi_layer_surface_subsurface_destroy_notify(struct wl_listener* listener,
                                            void* data)
{}

void
bsi_layer_surface_subsurface_map_notify(struct wl_listener* listener,
                                        void* data)
{}

void
bsi_layer_surface_subsurface_unmap_notify(struct wl_listener* listener,
                                          void* data)
{}
