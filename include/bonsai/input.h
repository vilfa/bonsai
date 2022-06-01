#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

struct bsi_input_pointer
{
    struct bsi_server* server;
    struct wlr_cursor* cursor;
    struct wlr_input_device* device;

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

    struct wl_list link_server; // bsi_server
};

struct bsi_input_keyboard
{
    struct bsi_server* server;
    struct wlr_input_device* device;

    struct
    {
        /* wlr_input_device::keyboard */
        struct wl_listener key;
        struct wl_listener modifiers;
        /* wlr_input_device::destroy */
        struct wl_listener destroy;
    } listen;

    struct wl_list link_server; // bsi_server
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

struct bsi_input_pointer*
bsi_input_pointer_init(struct bsi_input_pointer* pointer,
                       struct bsi_server* server,
                       struct wlr_input_device* device);

void
bsi_input_pointer_destroy(struct bsi_input_pointer* pointer);

struct bsi_input_keyboard*
bsi_input_keyboard_init(struct bsi_input_keyboard* keyboard,
                        struct bsi_server* server,
                        struct wlr_input_device* device);

void
bsi_input_keyboard_destroy(struct bsi_input_keyboard* keyboard);

void
bsi_input_keyboard_keymap_set(struct bsi_input_keyboard* keyboard,
                              const struct xkb_rule_names* xkb_rule_names,
                              const size_t xkb_rule_names_len);
