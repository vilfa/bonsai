#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

/**
 * @brief Holds all inputs the server knows about via signal listeners. The
 * inputs_pointers list holds elements of type `struct bsi_input_pointer`. The
 * inputs_keyboards list holds elements of type `struct bsi_input_keyboard`.
 *
 */
struct bsi_inputs
{
    struct bsi_server* bsi_server;
    struct wlr_seat* wlr_seat;

    size_t len_pointers;
    struct wl_list pointers;

    size_t len_keyboards;
    struct wl_list keyboards;
};

/**
 * @brief Holds a single input pointer and its event listeners.
 *
 */
struct bsi_input_pointer
{
    struct bsi_server* bsi_server;
    struct wlr_cursor* wlr_cursor;
    struct wlr_input_device* wlr_input_device;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    // TODO: What is actually necessary here?
    struct
    {
        /* wlr_cursor */
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
    } listen;

    struct wl_list link;
};

/**
 * @brief Holds a single input keyboard and its event listeners.
 *
 */
struct bsi_input_keyboard
{
    struct bsi_server* bsi_server;
    struct wlr_input_device* wlr_input_device;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    struct
    {
        /* wlr_input_device::keyboard */
        struct wl_listener key;
        struct wl_listener modifiers;
        struct wl_listener keymap;
        struct wl_listener repeat_info;
        struct wl_listener destroy;
    } listen;

    struct wl_list link;
};

/**
 * @brief Identifies all currently supported xkb layouts.
 *
 */
enum bsi_input_keyboard_layout
{
    BSI_INPUT_KEYBOARD_LAYOUT_EN_US,
    BSI_INPUT_KEYBOARD_LAYOUT_SI_SI,
    BSI_INPUT_KEYBOARD_LAYOUT_SI_US,
};

#define bsi_input_keyboard_rules_len 3

/**
 * @brief Holds all currently supported xkb layout rules.
 *
 */
// ! For documentation on this illusive thing, see xkeyboard-config(7).
// TODO: Make this configuration available from somewhere.
static const struct xkb_rule_names bsi_input_keyboard_rules[] = {
    [BSI_INPUT_KEYBOARD_LAYOUT_EN_US] = {
        .rules = NULL,
        .model = "pc105",
        .layout = "us",
        .variant = NULL,
        .options = "grp:win_space_toggle",
    },
    [BSI_INPUT_KEYBOARD_LAYOUT_SI_SI] = {
        .rules = NULL,
        .model = "pc105",
        .layout = "si",
        .variant = NULL,
        .options = "grp:win_space_toggle",
    },
    [BSI_INPUT_KEYBOARD_LAYOUT_SI_US] = {
        .rules = NULL,
        .model = "pc105",
        .layout = "si(us)",
        .variant = NULL,
        .options = "grp:win_space_toggle",
    },
};

/**
 * @brief Initializes inputs for the given server seat.
 *
 * @param bsi_inputs Pointer to bsi_inputs server struct.
 * @param bsi_server Pointer to the server.
 * @return struct bsi_inputs* Pointer to initialized server seat.
 */
struct bsi_inputs*
bsi_inputs_init(struct bsi_inputs* bsi_inputs, struct bsi_server* bsi_server);

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
 * @brief Unlinks all active listeners for the specified `bsi_input_pointer`.
 *
 * @param bsi_input_pointer The input pointer.
 */
void
bsi_input_pointer_finish(struct bsi_input_pointer* bsi_input_pointer);

/**
 * @brief Destroys (calls `free`) on the input pointer.
 *
 * @param bsi_input_pointer The input pointer to destroy.
 */
void
bsi_input_pointer_destroy(struct bsi_input_pointer* bsi_input_pointer);

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
 * @brief Unlinks all active listeners for the specified `bsi_input_keyboard`.
 *
 * @param bsi_input_keyboard The input keyboard.
 */
void
bsi_input_keyboard_finish(struct bsi_input_keyboard* bsi_input_keyboard);

/**
 * @brief Destroys (calls `free`) on the keyboard.
 *
 * @param bsi_input_keyboard The keyboard to destroy.
 */
void
bsi_input_keyboard_destroy(struct bsi_input_keyboard* bsi_input_keyboard);

/**
 * @brief Gets a keymap from the xkb context and sets it for the specified
 * keyboard.
 *
 * @param bsi_input_keyboard The keyboard.
 * @param xkb_rule_names The xkb keymap rules.
 */
void
bsi_input_keyboard_keymap_set(struct bsi_input_keyboard* bsi_input_keyboard,
                              const struct xkb_rule_names* xkb_rule_names,
                              const size_t xkb_rule_names_len);
