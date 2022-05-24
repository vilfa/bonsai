#pragma once

#include "bonsai/config/input.h"
#include "bonsai/config/output.h"
#include "bonsai/config/signal.h"
#include "bonsai/desktop/cursor.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"

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
    struct bsi_cursor bsi_cursor;
    struct bsi_workspaces bsi_workspaces;

    // TODO
    /* So, the way I imagine it, a server will have a bunch of workspaces, maybe
     * with another parent to hold the workspaces. The panels or panel,
     * probably, will then exist outside of the workspaces, and will be
     * positioned on the top and maybe bottom of the screen. The top panel will
     * hold a workspace indicator, a datetime indicator, a system tray, a switch
     * between floating and tiling mode (this is optional, maybe I will just
     * leave everything floating, as it is the usual desktop experience). Now, I
     * know that there is a lot of work to be done before any of this, but a
     * somewhat clear plan probably helps. */
};

/**
 * @brief Initializes the server.
 *
 * @param bsi_server The server.
 * @return struct bsi_server* The initialized server.
 */
struct bsi_server*
bsi_server_init(struct bsi_server* bsi_server);

/**
 * @brief Cleans everything up and exists with 0.
 *
 * @param bsi_server The server.
 */
void
bsi_server_exit(struct bsi_server* bsi_server);
