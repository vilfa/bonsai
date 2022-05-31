#pragma once

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>

#include "bonsai/output.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

enum bsi_layer_surface_type
{
    BSI_LAYER_SURFACE_TOPLEVEL,
    BSI_LAYER_SURFACE_POPUP,
    BSI_LAYER_SURFACE_SUBSURFACE,
};

struct bsi_layer_surface_toplevel;
struct bsi_layer_surface_popup;
struct bsi_layer_surface_subsurface;

/**
 * @brief Just a helpful union so we don't have to have functions for all
 * types of layers.
 *
 */
union bsi_layer_surface
{
    struct bsi_layer_surface_toplevel* toplevel;
    struct bsi_layer_surface_popup* popup;
    struct bsi_layer_surface_subsurface* subsurface;
};

struct bsi_layer_surface_toplevel
{
    struct bsi_output* output;
    struct wlr_layer_surface_v1* layer_surface;
    struct wlr_scene_layer_surface_v1* scene_node;
    struct wl_list subsurfaces;

    bool mapped;
    struct wlr_box box, extent;

    /* Am member of this type of layer. This will be passed in
     * wlr_layer_surface::pending when a client is configuring. This is the type
     * of `wlr_layer_surface`. */
    enum zwlr_layer_shell_v1_layer member_of_type;

    struct
    {
        /* wlr_layer_surface_v1 -> layer_surface */
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        struct wl_listener new_popup;
        /* wlr_surface -> layer_surface::surface */
        struct wl_listener commit;
        struct wl_listener new_subsurface;
    } listen;

    struct wl_list link;
};

struct bsi_layer_surface_popup
{
    struct wlr_xdg_popup* popup;

    enum bsi_layer_surface_type parent_type;
    union bsi_layer_surface parent; /* Tagged by `parent_type`. */

    struct
    {
        /* wlr_xdg_surface -> popup::base */
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        struct wl_listener new_popup;
        /* wlr_surface -> popup::base::surface */
        struct wl_listener commit;
    } listen;
};

struct bsi_layer_surface_subsurface
{
    struct wlr_subsurface* subsurface;

    /* Am member of this layer_surface. */
    struct bsi_layer_surface_toplevel* member_of;

    struct
    {
        /* wlr_subsurface -> subsurface */
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        /* wlr_surface -> subsurface::surface */
        struct wl_listener commit;
    } listen;

    struct wl_list link;
};

void
bsi_layers_add(struct bsi_output* output,
               struct bsi_layer_surface_toplevel* surface_toplevel,
               enum zwlr_layer_shell_v1_layer at_layer);

void
bsi_layers_arrange(struct bsi_output* output);

struct bsi_layer_surface_toplevel*
bsi_layer_surface_toplevel_init(struct bsi_layer_surface_toplevel* toplevel,
                                struct wlr_layer_surface_v1* layer_surface,
                                struct bsi_output* output);

struct bsi_layer_surface_popup*
bsi_layer_surface_popup_init(struct bsi_layer_surface_popup* popup,
                             struct wlr_xdg_popup* xdg_popup,
                             enum bsi_layer_surface_type parent_type,
                             union bsi_layer_surface parent);

struct bsi_layer_surface_subsurface*
bsi_layer_surface_subsurface_init(
    struct bsi_layer_surface_subsurface* subsurface,
    struct wlr_subsurface* wlr_subsurface,
    struct bsi_layer_surface_toplevel* member_of);

struct bsi_layer_surface_toplevel*
bsi_layer_surface_get_toplevel_parent(union bsi_layer_surface layer_surface,
                                      enum bsi_layer_surface_type type);

void
bsi_layer_surface_focus(struct bsi_layer_surface_toplevel* toplevel);

void
bsi_layer_surface_destroy(union bsi_layer_surface layer_surface,
                          enum bsi_layer_surface_type type);
