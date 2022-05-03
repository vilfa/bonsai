#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

/**
 * @brief Gives information about the active listeners in bsi_listeners.
 *
 */
enum bsi_listeners_mask
{
    BSI_LISTENER_OUTPUT = 1 << 0,
    BSI_LISTENER_INPUT = 1 << 1,
};

/**
 * @brief Holds all signal listeners that belong to the server.
 *
 */
struct bsi_listeners
{
    struct bsi_server* server;

    uint32_t active_listeners;

    struct wl_listener new_output_listener;
    struct wl_listener new_input_listener;
};

struct bsi_listeners*
bsi_listeners_init(struct bsi_listeners* bsi_listeners);

void
bsi_listeners_add_new_output_notify(struct bsi_listeners* bsi_listeners,
                                    wl_notify_func_t func);

void
bsi_listeners_add_new_input_notify(struct bsi_listeners* bsi_listeners,
                                   wl_notify_func_t func);
