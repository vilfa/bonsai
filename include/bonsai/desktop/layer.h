#pragma once

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "wlr-layer-shell-unstable-v1-protocol.h"

enum bsi_layers_listener_mask
{
    BSI_LAYERS_LISTENER_MAP = 1 << 0,
    BSI_LAYERS_LISTENER_UNMAP = 1 << 1,
    BSI_LAYERS_LISTENER_DESTROY = 1 << 2,
    BSI_LAYERS_LISTENER_NEW_POPUP = 1 << 3,
};

#define bsi_layers_listener_len 4

/**
 * @brief Holds all layers that belong to a single output.
 *
 */
struct bsi_layers
{
    struct bsi_server* bsi_server;
    struct bsi_output* bsi_output;

    uint32_t active_listeners;
    struct wl_list* active_links[bsi_layers_listener_len];
    size_t len_active_links;
    struct
    {
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        struct wl_listener new_popup;
    } events;

    uint32_t full_layers;
    struct wlr_layer_surface_v1 layers[4];
};

struct bsi_layers*
bsi_layers_init(struct bsi_layers* bsi_layers);
