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
    BSI_LISTENER_XDG_SURFACE = 1 << 2,
    BSI_LISTENER_REQUEST_SET_CURSOR = 1 << 3,
    BSI_LISTENER_REQUEST_SET_SELECTION = 1 << 4,
};

/**
 * @brief Holds all signal listeners that belong to the server.
 *
 */
struct bsi_listeners
{
    struct bsi_server* bsi_server;

    uint32_t active_listeners;

    /* bsi_outputs */
    struct wl_listener new_output;
    /* bsi_inputs / wlr_seat */
    struct wl_listener new_input;
    // TODO: Add handlers for these two events.
    struct wl_listener request_set_cursor;
    struct wl_listener request_set_selection;
    /* wlr_xdg_surface */
    struct wl_listener new_xdg_surface;

    // TODO: Add handlers for wlr_backend->events
    // TODO: Add handlers for wlr_seat->events
    // TODO: Add handlers for wlr_xdg_shell->events
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
 * @brief Adds a `new output` listener to the server listeners.
 *
 * @param bsi_listeners Pointer to server listeners.
 * @param func Listener function.
 */
void
bsi_listeners_add_new_output_notify(struct bsi_listeners* bsi_listeners,
                                    wl_notify_func_t func);

/**
 * @brief Adds a `new input` listener to the server listeners.
 *
 * @param bsi_listeners Pointer to server listeners.
 * @param func Listener function.
 */
void
bsi_listeners_add_new_input_notify(struct bsi_listeners* bsi_listeners,
                                   wl_notify_func_t func);

/**
 * @brief Adds a `request set cursor` listener to the server listeners.
 *
 * @param bsi_listeners Pointer to server listeners.
 * @param func Listener function.
 */
void
bsi_listeners_add_request_set_cursor_notify(struct bsi_listeners* bsi_listeners,
                                            wl_notify_func_t func);

/**
 * @brief Adds a `request set selection` listener to the server listeners.
 *
 * @param bsi_listeners Pointer to server listeners.
 * @param func Listener function.
 */
void
bsi_listeners_add_request_set_selection_notify(
    struct bsi_listeners* bsi_listeners,
    wl_notify_func_t func);

/**
 * @brief Adds a `new xdg surface` listener to the server listeners.
 *
 * @param bsi_listeners Pointer to server listeners.
 * @param func Listener func.
 */
void
bsi_listeners_add_new_xdg_surface_notify(struct bsi_listeners* bsi_listeners,
                                         wl_notify_func_t func);
