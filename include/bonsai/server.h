#pragma once

#include "bonsai/config/output.h"
#include "bonsai/config/signal.h"

/**
 * @brief Represents the compositor and its internal state.
 *
 */
struct bsi_server
{
    /* Globals */
    struct wl_display* wl_display;
    const char* wl_socket;
    struct wl_event_loop* wl_event_loop;
    struct wlr_backend* wlr_backend;
    struct wlr_renderer* wlr_renderer;
    struct wlr_allocator* wlr_allocator;
    struct wlr_output_layout* wlr_output_layout;
    struct wlr_scene* wlr_scene;
    struct wlr_compositor* wlr_compositor;
    struct wlr_data_device_manager* wlr_data_device_manager;
    struct wlr_xdg_shell* wlr_xdg_shell;
    /* State */
    struct bsi_listeners bsi_listeners;
    struct bsi_outputs bsi_outputs;
};
