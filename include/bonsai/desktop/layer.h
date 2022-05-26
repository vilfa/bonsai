#pragma once

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "wlr-layer-shell-unstable-v1-protocol.h"

/**
 * So if I get this correctly, a layer is basically just another type of
 * surface, meaning that it should be structured in much the same way as a view,
 * with some common types of listeners.
 *
 */

/**
 * @brief Is just a wrapper that the output owns.
 *
 */
struct bsi_output_layers
{
    /* Basically an ad-hoc map indexable by `enum zwlr_layer_shell_v1_layer`
     * ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND = 0,
     * ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM = 1,
     * ZWLR_LAYER_SHELL_V1_LAYER_TOP = 2,
     * ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY = 3, */

    struct wl_list layers[4];
};

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
    struct wlr_layer_surface_v1* wlr_layer_surface;
    struct wl_list subsurfaces;

    /* Am member of this type of layer. This will be passed in
     * wlr_layer_surface::pending when a client is configuring. This is the type
     * of `wlr_layer_surface`. */
    enum zwlr_layer_shell_v1_layer member_of_type;

    struct
    {
        /* wlr_layer_surface_v1 */
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        struct wl_listener new_popup;
        /* wlr_surface -> wlr_layer_surface::surface */
        struct wl_listener wlr_surface_commit;
        struct wl_listener wlr_surface_new_subsurface;
        struct wl_listener wlr_surface_destroy; // TODO: Unnecessary?
    } listen;

    struct wl_list link;
};

struct bsi_layer_surface_popup
{
    struct wlr_xdg_popup* wlr_xdg_popup;

    enum bsi_layer_surface_type parent_type;
    union bsi_layer_surface parent; /* Tagged by `parent_type`. */

    struct
    {
        /* wlr_xdg_surface -> wlr_xdg_popup::base */
        struct wl_listener destroy;
        struct wl_listener ping_timeout; // TODO: Unnecessary?
        struct wl_listener new_popup;
        struct wl_listener map;
        struct wl_listener unmap;
    } listen;
};

struct bsi_layer_surface_subsurface
{
    struct wlr_subsurface* wlr_subsurface;

    /* Am member of this layer_surface. */
    struct bsi_layer_surface_toplevel* member_of;

    struct
    {
        /* wlr_subsurface */
        struct wl_listener destroy;
        struct wl_listener map;
        struct wl_listener unmap;
    } listen;

    struct wl_list link;
};

/* This does nothing currently, but it might do something later on. */
struct bsi_output_layers*
bsi_output_layers_init(struct bsi_output_layers* bsi_output_layers);

void
bsi_output_layers_destroy(struct bsi_output_layers* bsi_output_layers);

struct bsi_layer_surface_toplevel*
bsi_layer_surface_toplevel_init(
    struct bsi_layer_surface_toplevel* layer_surface,
    struct wlr_layer_surface_v1* wlr_layer_surface);

struct bsi_layer_surface_popup*
bsi_layer_surface_popup_init(struct bsi_layer_surface_popup* layer_surface,
                             struct wlr_xdg_popup* wlr_xdg_popup,
                             enum bsi_layer_surface_type parent_type,
                             union bsi_layer_surface parent);

struct bsi_layer_surface_subsurface*
bsi_layer_surface_subsurface_init(
    struct bsi_layer_surface_subsurface* layer_subsurface,
    struct wlr_subsurface* wlr_subsurface,
    struct bsi_layer_surface_toplevel* member_of);

void
bsi_layer_surface_finish(union bsi_layer_surface bsi_layer_surface,
                         enum bsi_layer_surface_type layer_surface_type);