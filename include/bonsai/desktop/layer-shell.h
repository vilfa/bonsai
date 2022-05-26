#pragma once

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "wlr-layer-shell-unstable-v1-protocol.h"

/**
 * @brief Holds all layers that belong to a single output.
 *
 */
struct bsi_layers
{
    struct bsi_server* bsi_server;
    struct bsi_output* bsi_output;

    size_t len_active_listen;
    struct
    {
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        struct wl_listener new_popup;
    } listen;

    uint32_t full_layers;
    struct wlr_layer_surface_v1 layers[4];
};

struct bsi_layer_surface
{};

struct bsi_layer_popup
{};

struct bsi_layer_subsurface
{};

struct bsi_layers*
bsi_layers_init(struct bsi_layers* bsi_layers);

void
bsi_layers_listener_add(struct bsi_layers* bsi_layers,
                        struct wl_listener* bsi_listener_memb,
                        struct wl_signal* bsi_signal_memb,
                        wl_notify_func_t* func);

void
bsi_layers_finish(struct bsi_layers* bsi_layers);
