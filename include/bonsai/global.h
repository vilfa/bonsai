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
    size_t len_active_listen;
    // TODO: What is actually necessary here?
    struct
    {
        /* wlr_backend */
        struct wl_listener wlr_backend_new_output;
        struct wl_listener wlr_backend_new_input;
        struct wl_listener wlr_backend_destroy;
        /* wlr_seat */
        struct wl_listener wlr_seat_pointer_grab_begin;
        struct wl_listener wlr_seat_pointer_grab_end;
        struct wl_listener wlr_seat_keyboard_grab_begin;
        struct wl_listener wlr_seat_keyboard_grab_end;
        struct wl_listener wlr_seat_touch_grab_begin;
        struct wl_listener wlr_seat_touch_grab_end;
        struct wl_listener wlr_seat_request_set_cursor;
        struct wl_listener wlr_seat_request_set_selection;
        struct wl_listener wlr_seat_set_selection;
        struct wl_listener wlr_seat_request_set_primary_selection;
        struct wl_listener wlr_seat_set_primary_selection;
        struct wl_listener wlr_seat_request_start_drag;
        struct wl_listener wlr_seat_start_drag;
        struct wl_listener wlr_seat_destroy;
        /* wlr_xdg_shell */
        struct wl_listener wlr_xdg_shell_new_surface;
        struct wl_listener wlr_xdg_shell_destroy;
        /* bsi_workspace */
        struct wl_listener bsi_workspace_active;
    } listen;
};

/**
 * @brief Initializes the server listeners struct.
 *
 * @param bsi_listeners_global Pointer to server listeners struct.
 * @return struct bsi_listeners* Pointer to initialized listeners struct.
 */
struct bsi_listeners_global*
bsi_listeners_global_init(struct bsi_listeners_global* bsi_listeners_global);

/**
 * @brief A generic function that support adding any type of listener defined in
 * `bsi_listeners_mask` to the server listeners.
 *
 * @param bsi_listeners_global Pointer to server listeners struct.
 * @param bsi_listener_type Type of listener to add.
 * @param bsi_listeners_memb Pointer to a listener to initialize with func (a
 * member of one of the `bsi_listeners` anonymus structs).
 * @param bsi_signal_memb Pointer to signal which the listener handles (usually
 * a member of the `events` struct of its parent).
 * @param func The listener func.
 */
void
bsi_listeners_global_add(struct bsi_listeners_global* bsi_listeners_global,
                         struct wl_listener* bsi_listener_memb,
                         struct wl_signal* bsi_signal_memb,
                         wl_notify_func_t func);

/**
 * @brief Unlinks all active listeners belonging to the server.
 *
 * @param bsi_listeners_global The listeners struct.
 */
void
bsi_listeners_global_finish(struct bsi_listeners_global* bsi_listeners_global);
