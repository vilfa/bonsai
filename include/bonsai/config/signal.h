#pragma once

#include <stdint.h>
#include <wayland-server-core.h>
#include <wayland-server.h>

/**
 * @brief Holds all possible listener types in `bsi_listeners`.
 *
 */
enum bsi_listeners_mask
{
    /* wlr_backend */
    BSI_LISTENERS_BACKEND_NEW_OUTPUT = 1 << 0,
    BSI_LISTENERS_BACKEND_NEW_INPUT = 1 << 1,
    BSI_LISTENERS_BACKEND_DESTROY = 1 << 2,
    /* wlr_seat */
    BSI_LISTENERS_SEAT_REQUEST_SET_CURSOR = 1 << 3,
    BSI_LISTENERS_SEAT_REQUEST_SET_SELECTION = 1 << 4,
    /* wlr_xdg_shell */
    BSI_LISTENERS_XDG_SHELL_NEW_SURFACE = 1 << 5,
};

/**
 * @brief Holds signal listeners for all the globals that belong to the server.
 *
 */
struct bsi_listeners
{
    struct bsi_server* bsi_server;
    uint32_t active_listeners;

    // TODO: Add handlers for these events.
    struct
    {
        struct wl_listener new_output;
        struct wl_listener new_input;
        struct wl_listener destroy; // TODO
    } wlr_backend;

    // TODO: Add handlers for these events.
    struct
    {
        struct wl_listener pointer_grab_begin; // TODO
        struct wl_listener pointer_grab_end;   // TODO

        struct wl_listener keyboard_grab_begin; // TODO
        struct wl_listener keyboard_grab_end;   // TODO

        struct wl_listener touch_grab_begin; // TODO
        struct wl_listener touch_grab_end;   // TODO

        struct wl_listener request_set_cursor; // TODO

        struct wl_listener request_set_selection; // TODO
        struct wl_listener set_selection;         // TODO

        struct wl_listener request_set_primary_selection; // TODO
        struct wl_listener set_primary_selection;         // TODO

        struct wl_listener request_start_drag; // TODO
        struct wl_listener start_drag;         // TODO

        struct wl_listener destroy; // TODO
    } wlr_seat;

    // TODO: Add handlers for these events.
    struct
    {
        struct wl_listener new_surface;
        struct wl_listener destroy; // TODO
    } wlr_xdg_shell;
};

/**
 * @brief Initializes the server listeners struct.
 *
 * @param bsi_listeners Pointer to server listeners struct.
 * @return struct bsi_listeners* Pointer to initialized listeners struct.
 */
struct bsi_listeners*
bsi_listeners_init(struct bsi_listeners* bsi_listeners);

/**
 * @brief A generic function that support adding any type of listener defined in
 * `bsi_listeners_mask` to the server listeners.
 *
 * @param bsi_listeners Pointer to server listeners struct.
 * @param bsi_listener_type Type of listener to add.
 * @param bsi_listeners_memb Pointer to a listener to initialize with func (a
 * member of one of the `bsi_listeners` anonymus structs).
 * @param bsi_signal_memb Pointer to signal which the listener handles (usually
 * a member of the `events` struct of its parent).
 * @param func The listener func.
 */
void
bsi_listeners_add_listener(struct bsi_listeners* bsi_listeners,
                           enum bsi_listeners_mask bsi_listener_type,
                           struct wl_listener* bsi_listener_memb,
                           struct wl_signal* bsi_signal_memb,
                           wl_notify_func_t func);
