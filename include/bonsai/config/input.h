#pragma once

#include <stddef.h>
#include <stdint.h>
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
    struct wlr_seat* wlr_seat;

    size_t len_pointers;
    struct wl_list pointers;

    size_t len_keyboards;
    struct wl_list keyboards;
};

/**
 * @brief Holds all possible listener types for `bsi_input_pointer`.
 *
 */
enum bsi_input_pointer_listener_mask
{
    BSI_INPUT_POINTER_LISTENER_MOTION = 1 << 0,
    BSI_INPUT_POINTER_LISTENER_MOTION_ABSOLUTE = 1 << 1,
    BSI_INPUT_POINTER_LISTENER_BUTTON = 1 << 2,
    BSI_INPUT_POINTER_LISTENER_AXIS = 1 << 3,
    BSI_INPUT_POINTER_LISTENER_FRAME = 1 << 4,
    BSI_INPUT_POINTER_LISTENER_SWIPE_BEGIN = 1 << 5,
    BSI_INPUT_POINTER_LISTENER_SWIPE_UPDATE = 1 << 6,
    BSI_INPUT_POINTER_LISTENER_SWIPE_END = 1 << 7,
    BSI_INPUT_POINTER_LISTENER_PINCH_BEGIN = 1 << 8,
    BSI_INPUT_POINTER_LISTENER_PINCH_UPDATE = 1 << 9,
    BSI_INPUT_POINTER_LISTENER_PINCH_END = 1 << 10,
    BSI_INPUT_POINTER_LISTENER_HOLD_BEGIN = 1 << 11,
    BSI_INPUT_POINTER_LISTENER_HOLD_END = 1 << 12,
};

#define bsi_input_pointer_listener_len 13

/**
 * @brief Holds a single input pointer and its event listeners.
 *
 */
struct bsi_input_pointer
{
    struct bsi_server* bsi_server;
    struct wlr_cursor* wlr_cursor;
    struct wlr_input_device* wlr_input_device;

    uint32_t active_listeners;
    struct wl_list* active_links[bsi_input_pointer_listener_len];
    size_t len_active_links;
    struct
    {
        struct wl_listener motion;
        struct wl_listener motion_absolute;
        struct wl_listener button;
        struct wl_listener axis;
        struct wl_listener frame;
        struct wl_listener swipe_begin;
        struct wl_listener swipe_update;
        struct wl_listener swipe_end;
        struct wl_listener pinch_begin;
        struct wl_listener pinch_update;
        struct wl_listener pinch_end;
        struct wl_listener hold_begin;
        struct wl_listener hold_end;
    } events;

    struct wl_list link;
};

/**
 * @brief Holds all possible listener types for `bsi_input_keyboard`.
 *
 */
enum bsi_input_keyboard_listener_mask
{
    BSI_INPUT_KEYBOARD_LISTENER_KEY = 1 << 0,
    BSI_INPUT_KEYBOARD_LISTENER_MODIFIERS = 1 << 1,
    BSI_INPUT_KEYBOARD_LISTENER_KEYMAP = 1 << 2,
    BSI_INPUT_KEYBOARD_LISTENER_REPEAT_INFO = 1 << 3,
    BSI_INPUT_KEYBOARD_LISTENER_DESTROY = 1 << 4,
};

#define bsi_input_keyboard_listener_len 5

/**
 * @brief Holds a single input keyboard and its event listeners.
 *
 */
struct bsi_input_keyboard
{
    struct bsi_server* bsi_server;
    struct wlr_input_device* wlr_input_device;

    uint32_t active_listeners;
    struct wl_list* active_links[bsi_input_keyboard_listener_len];
    size_t len_active_links;
    struct
    {
        struct wl_listener key;
        struct wl_listener modifiers;
        struct wl_listener keymap;
        struct wl_listener repeat_info;
        struct wl_listener destroy;
    } events;

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
bsi_inputs_pointer_add(struct bsi_inputs* bsi_inputs,
                       struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Remove a pointer from the server inputs. Make sure to destroy the
 * pointer.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_pointer Pointer to pointer to remove.
 */
void
bsi_inputs_pointer_remove(struct bsi_inputs* bsi_inputs,
                          struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Add a keyboard to the server inputs.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_keyboard Pointer to keyboard to add.
 */
void
bsi_inputs_keyboard_add(struct bsi_inputs* bsi_inputs,
                        struct bsi_input_keyboard* bsi_input_keyboard);

/**
 * @brief Remove a keyboard from the server struct. Make sure to destroy the
 * keyboard.
 *
 * @param bsi_inputs Pointer to server inputs struct.
 * @param bsi_input_keyboard Pointer to keyboard to remove.
 */
void
bsi_inputs_keyboard_remove(struct bsi_inputs* bsi_inputs,
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
bsi_inputs_len_keyboards(struct bsi_inputs* bsi_inputs);

/**
 * @brief Initializes a preallocated `bsi_input_pointer`.
 *
 * @param bsi_input_pointer Input pointer to initialize.
 * @param bsi_server The server.
 * @param wlr_input_device Input device data.
 * @return struct bsi_input_pointer* Pointer to initialized struct.
 */
struct bsi_input_pointer*
bsi_input_pointer_init(struct bsi_input_pointer* bsi_input_pointer,
                       struct bsi_server* bsi_server,
                       struct wlr_input_device* wlr_input_device);

/**
 * @brief Destroys (calls `free`) on the input pointer.
 *
 * @param bsi_input_pointer The input pointer to destroy.
 */
void
bsi_input_pointer_destroy(struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Adds a listener `func` for the specified member of the
 * `bsi_input_pointer` `events` struct.
 *
 * @param bsi_input_pointer The input pointer.
 * @param bsi_listener_type Type of listener to add.
 * @param bsi_listener_memb Pointer to a listener to initialize with func (a
 * member of the `events` anonymus struct).
 * @param bsi_signal_memb Pointer to signal which the listener handles (usually
 * a member of the `events` struct of its parent).
 * @param func The listener function.
 */
void
bsi_input_pointer_listener_add(
    struct bsi_input_pointer* bsi_input_pointer,
    enum bsi_input_pointer_listener_mask bsi_listener_type,
    struct wl_listener* bsi_listener_memb,
    struct wl_signal* bsi_signal_memb,
    wl_notify_func_t func);

/**
 * @brief Unlinks all active listeners for the specified `bsi_input_pointer`.
 *
 * @param bsi_input_pointer The input pointer.
 */
void
bsi_input_pointer_listeners_unlink_all(
    struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Initializes a preallocated `bsi_input_keyboard`.
 *
 * @param bsi_input_keyboard The input keyboard.
 * @param bsi_server The server.
 * @param wlr_input_device Input device data.
 * @return struct bsi_input_keyboard*
 */
struct bsi_input_keyboard*
bsi_input_keyboard_init(struct bsi_input_keyboard* bsi_input_keyboard,
                        struct bsi_server* bsi_server,
                        struct wlr_input_device* wlr_input_device);

/**
 * @brief Destroys (calls `free`) on the keyboard.
 *
 * @param bsi_input_keyboard The keyboard to destroy.
 */
void
bsi_input_keyboard_destroy(struct bsi_input_keyboard* bsi_input_keyboard);

/**
 * @brief Adds a listener `func` for the specified member of the
 * `bsi_input_keyboard` `events` struct.
 *
 * @param bsi_input_keyboard The input keyboard.
 * @param bsi_listener_type Type of listener to add.
 * @param bsi_listener_memb Pointer to a listener to initialize with func (a
 * member of the `events` anonymus struct).
 * @param bsi_signal_memb Pointer to signal which the listener handles
 * (usually a member of the `events` struct of its parent).
 * @param func The listener function.
 */
void
bsi_input_keyboard_listener_add(
    struct bsi_input_keyboard* bsi_input_keyboard,
    enum bsi_input_keyboard_listener_mask bsi_listener_type,
    struct wl_listener* bsi_listener_memb,
    struct wl_signal* bsi_signal_memb,
    wl_notify_func_t func);

/**
 * @brief Unlinks all active listeners for the specified `bsi_input_keyboard`.
 *
 * @param bsi_input_keyboard The input keyboard.
 */
void
bsi_input_keyboard_listeners_unlink_all(
    struct bsi_input_keyboard* bsi_input_keyboard);
