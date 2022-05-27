#pragma once

#include <stdint.h>
#include <wayland-server-core.h>
#include <wayland-server.h>

/**
 * @brief Holds signal listeners for all the globals that belong to the server.
 *
 */
struct bsi_listeners_global
{
    struct bsi_server* bsi_server;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    struct
    {
        /* wlr_backend */
        struct wl_listener wlr_backend_new_output;
        struct wl_listener wlr_backend_new_input;
        /* wlr_seat */
        struct wl_listener wlr_seat_pointer_grab_begin;
        struct wl_listener wlr_seat_pointer_grab_end;
        struct wl_listener wlr_seat_keyboard_grab_begin;
        struct wl_listener wlr_seat_keyboard_grab_end;
        struct wl_listener wlr_seat_touch_grab_begin;
        struct wl_listener wlr_seat_touch_grab_end;
        struct wl_listener wlr_seat_request_set_cursor;
        struct wl_listener wlr_seat_request_set_selection;
        struct wl_listener wlr_seat_request_set_primary_selection;
        struct wl_listener wlr_seat_request_start_drag;
        struct wl_listener wlr_seat_start_drag;
        /* wlr_xdg_shell */
        struct wl_listener wlr_xdg_shell_new_surface;
        /* wlr_layer_shell_v1 */
        struct wl_listener wlr_layer_shell_new_surface;
        /* bsi_workspace */
        struct wl_listener bsi_workspace_active;
    } listen;
};

/**
 * @brief Initializes the server listeners struct.
 *
 * @param bsi_listeners_global Pointer to server listeners struct.
 * @param bsi_server The server.
 * @return struct bsi_listeners* Pointer to initialized listeners struct.
 */
struct bsi_listeners_global*
bsi_listeners_global_init(struct bsi_listeners_global* bsi_listeners_global,
                          struct bsi_server* bsi_server);

/**
 * @brief Unlinks all active listeners belonging to the server.
 *
 * @param bsi_listeners_global The listeners struct.
 */
void
bsi_listeners_global_finish(struct bsi_listeners_global* bsi_listeners_global);
