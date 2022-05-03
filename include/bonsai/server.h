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
    struct wl_event_loop* wl_event_loop;
    struct wlr_backend* wlr_backend;
    struct wlr_renderer* wlr_renderer;
    struct wlr_allocator* wlr_allocator;
    struct wlr_output_layout* wlr_output_layout;
    struct wlr_scene* wlr_scene;
    /* State */
    struct bsi_listeners bsi_listeners;
    struct bsi_outputs bsi_outputs;
};
