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
    BSI_LISTENERS_SEAT_POINTER_GRAB_BEGIN = 1 << 3,
    BSI_LISTENERS_SEAT_POINTER_GRAB_END = 1 << 4,
    BSI_LISTENERS_SEAT_KEYBOARD_GRAB_BEGIN = 1 << 5,
    BSI_LISTENERS_SEAT_KEYBOARD_GRAB_END = 1 << 6,
    BSI_LISTENERS_SEAT_TOUCH_GRAB_BEGIN = 1 << 7,
    BSI_LISTENERS_SEAT_TOUCH_GRAB_END = 1 << 8,
    BSI_LISTENERS_SEAT_REQUEST_SET_CURSOR = 1 << 9,
    BSI_LISTENERS_SEAT_REQUEST_SET_SELECTION = 1 << 10,
    BSI_LISTENERS_SEAT_SET_SELECTION = 1 << 11,
    BSI_LISTENERS_SEAT_REQUEST_SET_PRIMARY_SELECTION = 1 << 12,
    BSI_LISTENERS_SEAT_SET_PRIMARY_SELECTION = 1 << 13,
    BSI_LISTENERS_SEAT_REQUEST_START_DRAG = 1 << 14,
    BSI_LISTENERS_SEAT_START_DRAG = 1 << 15,
    BSI_LISTENERS_SEAT_DESTROY = 1 << 16,
    /* wlr_xdg_shell */
    BSI_LISTENERS_XDG_SHELL_NEW_SURFACE = 1 << 17,
    BSI_LISTENERS_XDG_SHELL_DESTROY = 1 << 18,
    /* bsi_workspace */
    BSI_LISTENERS_WORKSPACE_ACTIVE = 1 << 19,
};

#define bsi_listeners_len 19

/**
 * @brief Holds signal listeners for all the globals that belong to the server.
 *
 */
struct bsi_listeners
{
    struct bsi_server* bsi_server;

    uint32_t active_listeners;
    struct wl_list* active_links[bsi_listeners_len];
    size_t len_active_links;

    struct
    {
        struct wl_listener new_output;
        struct wl_listener new_input;
        struct wl_listener destroy;
    } wlr_backend;

    struct
    {
        struct wl_listener pointer_grab_begin;
        struct wl_listener pointer_grab_end;
        struct wl_listener keyboard_grab_begin;
        struct wl_listener keyboard_grab_end;
        struct wl_listener touch_grab_begin;
        struct wl_listener touch_grab_end;
        struct wl_listener request_set_cursor;
        struct wl_listener request_set_selection;
        struct wl_listener set_selection;
        struct wl_listener request_set_primary_selection;
        struct wl_listener set_primary_selection;
        struct wl_listener request_start_drag;
        struct wl_listener start_drag;
        struct wl_listener destroy;
    } wlr_seat;

    struct
    {
        struct wl_listener new_surface;
        struct wl_listener destroy;
    } wlr_xdg_shell;

    struct
    {
        struct wl_listener active;
    } bsi_workspace;
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
bsi_listeners_add(struct bsi_listeners* bsi_listeners,
                  enum bsi_listeners_mask bsi_listener_type,
                  struct wl_listener* bsi_listener_memb,
                  struct wl_signal* bsi_signal_memb,
                  wl_notify_func_t func);

/**
 * @brief Unlinks all active listeners belonging to the server.
 *
 * @param bsi_listeners The listeners struct.
 */
void
bsi_listeners_unlink_all(struct bsi_listeners* bsi_listeners);
