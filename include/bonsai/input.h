#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

/**
 * @brief Holds a single input pointer and its event listeners.
 *
 */
struct bsi_input_pointer
{
    struct bsi_server* server;
    struct wlr_cursor* cursor;
    struct wlr_input_device* device;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
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
        /* wlr_input_device::destroy */
        struct wl_listener destroy;
    } listen;

    struct wl_list link;
};

/**
 * @brief Holds a single input keyboard and its event listeners.
 *
 */
struct bsi_input_keyboard
{
    struct bsi_server* server;
    struct wlr_input_device* device;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    struct
    {
        /* wlr_input_device::keyboard */
        struct wl_listener key;
        struct wl_listener modifiers;
        /* wlr_input_device::destroy */
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
 * @brief Adds a pointer to the server inputs.
 *
 * @param server The server.
 * @param pointer Pointer to pointer to add.
 */
void
bsi_inputs_pointer_add(struct bsi_server* server,
                       struct bsi_input_pointer* pointer);

/**
 * @brief Remove a pointer from the server inputs. Make sure to destroy the
 * pointer.
 *
 * @param server The server.
 * @param pointer Pointer to pointer to remove.
 */
void
bsi_inputs_pointer_remove(struct bsi_server* server,
                          struct bsi_input_pointer* pointer);

/**
 * @brief Add a keyboard to the server inputs.
 *
 * @param server The server.
 * @param keyboard Pointer to keyboard to add.
 */
void
bsi_inputs_keyboard_add(struct bsi_server* server,
                        struct bsi_input_keyboard* keyboard);

/**
 * @brief Remove a keyboard from the server struct. Make sure to destroy the
 * keyboard.
 *
 * @param server The server.
 * @param keyboard Pointer to keyboard to remove.
 */
void
bsi_inputs_keyboard_remove(struct bsi_server* server,
                           struct bsi_input_keyboard* keyboard);

/**
 * @brief Initializes a preallocated `bsi_input_pointer`.
 *
 * @param pointer Input pointer to initialize.
 * @param server The server.
 * @param device Input device data.
 * @return struct bsi_input_pointer* Pointer to initialized struct.
 */
struct bsi_input_pointer*
bsi_input_pointer_init(struct bsi_input_pointer* pointer,
                       struct bsi_server* server,
                       struct wlr_input_device* device);

/**
 * @brief Unlinks all listeners and frees the pointer.
 *
 * @param pointer The input pointer to destroy.
 */
void
bsi_input_pointer_destroy(struct bsi_input_pointer* pointer);

/**
 * @brief Initializes a preallocated `bsi_input_keyboard`.
 *
 * @param keyboard The input keyboard.
 * @param server The server.
 * @param device Input device data.
 * @return struct bsi_input_keyboard*
 */
struct bsi_input_keyboard*
bsi_input_keyboard_init(struct bsi_input_keyboard* keyboard,
                        struct bsi_server* server,
                        struct wlr_input_device* device);

/**
 * @brief Unlinks all listeners and frees the keyboard.
 *
 * @param keyboard The keyboard to destroy.
 */
void
bsi_input_keyboard_destroy(struct bsi_input_keyboard* keyboard);

/**
 * @brief Gets a keymap from the xkb context and sets it for the specified
 * keyboard.
 *
 * @param keyboard The keyboard.
 * @param xkb_rule_names The xkb keymap rules.
 */
void
bsi_input_keyboard_keymap_set(struct bsi_input_keyboard* keyboard,
                              const struct xkb_rule_names* xkb_rule_names,
                              const size_t xkb_rule_names_len);
