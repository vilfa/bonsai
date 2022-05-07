#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

/**
 * @brief Holds all inputs the server knows about via signal listeners. The
 * inputs_pointers list holds elements of type `struct bsi_input_pointer`. The
 * inputs_keyboards list holds elements of type `struct bsi_input_keyboard`.
 *
 */
struct bsi_inputs
{
    struct wlr_seat* seat;

    size_t len_pointers;
    struct wl_list inputs_pointers;

    size_t len_keyboards;
    struct wl_list inputs_keyboards;
};

/**
 * @brief Holds a single input pointer and its event listeners.
 *
 */
struct bsi_input_pointer
{
    struct bsi_server* server;
    struct wlr_cursor* wlr_cursor;
    struct wlr_input_device* wlr_input_device;

    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;

    struct wl_list link;
};

/**
 * @brief Holds a single input keyboard and its event listeners.
 *
 */
struct bsi_input_keyboard
{
    struct bsi_server* server;
    struct wlr_input_device* wlr_input_device;

    struct wl_listener key;
    struct wl_listener modifier;

    struct wl_list link;
};

/**
 * @brief Initializes inputs for the given server seat.
 *
 * @param bsi_inputs Pointer to bsi_inputs server struct.
 * @param wlr_seat Pointer to the server seat.
 * @return struct bsi_inputs* Pointer to initialized server seat.
 */
struct bsi_inputs*
bsi_inputs_init(struct bsi_inputs* bsi_inputs, struct wlr_seat* wlr_seat);

/**
 * @brief Adds a pointer to the server inputs.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_pointer Pointer to pointer to add.
 */
void
bsi_inputs_add_pointer(struct bsi_inputs* bsi_inputs,
                       struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Remove a pointer from the server inputs.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_pointer Pointer to pointer to remove.
 */
void
bsi_inputs_remove_pointer(struct bsi_inputs* bsi_inputs,
                          struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Add a keyboard to the server inputs.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_keyboard Pointer to keyboard to add.
 */
void
bsi_inputs_add_keyboard(struct bsi_inputs* bsi_inputs,
                        struct bsi_input_keyboard* bsi_input_keyboard);

/**
 * @brief Remove a keyboard from the server struct.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_keyboard Pointer to keyboard to remove.
 */
void
bsi_inputs_remove_keyboard(struct bsi_inputs* bsi_inputs,
                           struct bsi_input_keyboard* bsi_input_keyboard);

/**
 * @brief Gets the number of server input pointers.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @return size_t The number of server input pointers.
 */
size_t
bsi_inputs_len_pointers(struct bsi_inputs* bsi_inputs);

/**
 * @brief Gets the number of server input keyboards.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @return size_t The number of server input keyboards.
 */
size_t
bsi_inputs_len_keyboard(struct bsi_inputs* bsi_inputs);
