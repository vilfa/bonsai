#pragma once

#include "bonsai/config/input.h"
#include "bonsai/config/output.h"
#include "bonsai/config/signal.h"
#include "bonsai/scene/view.h"

/**
 * @brief Represents the compositor and its internal state.
 *
 */
struct bsi_server
{
    /* Globals */
    const char* wl_socket;
    struct wl_display* wl_display;
    struct wl_event_loop* wl_event_loop;
    struct wlr_backend* wlr_backend;
    struct wlr_renderer* wlr_renderer;
    struct wlr_allocator* wlr_allocator;
    struct wlr_output_layout* wlr_output_layout;
    struct wlr_scene* wlr_scene;
    struct wlr_compositor* wlr_compositor;
    struct wlr_data_device_manager* wlr_data_device_manager;
    struct wlr_xdg_shell* wlr_xdg_shell;
    struct wlr_seat* wlr_seat;
    struct wlr_cursor* wlr_cursor;
    struct wlr_xcursor_manager* wlr_xcursor_manager;
    /* State */
    struct bsi_outputs bsi_outputs;
    struct bsi_inputs bsi_inputs;
    struct bsi_views bsi_views;
    struct bsi_listeners bsi_listeners;
};

// TODO: Implement this.
/**
 * @brief Initializes the server.
 *
 * @param bsi_server The server.
 * @return struct bsi_server* The initialized server.
 */
struct bsi_server*
bsi_server_init(struct bsi_server* bsi_server);
