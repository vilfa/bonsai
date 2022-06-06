#pragma once

#include <wlr/types/wlr_keyboard.h>

#include "bonsai/input.h"

/**
 * @brief Holds all possible modifier combos that the server might handle.
 *
 */
enum bsi_keyboard_modifier
{
    BSI_KEYBOARD_MOD_NONE = 1 << 0,
    BSI_KEYBOARD_MOD_CTRL = 1 << 1,
    BSI_KEYBOARD_MOD_ALT = 1 << 2,
    BSI_KEYBOARD_MOD_SUPER = 1 << 3,
    BSI_KEYBOARD_MOD_CTRL_ALT = 1 << 4,
    BSI_KEYBOARD_MOD_CTRL_SHIFT = 1 << 5,
    BSI_KEYBOARD_MOD_CTRL_ALT_SHIFT = 1 << 6,
    BSI_KEYBOARD_MOD_SUPER_SHIFT = 1 << 7,
};

/**
 * @brief Processes keybinds and calls handler. Returns appropriately.
 *
 * @param keyboard The input keyboard.
 * @param event The input event.
 * @return true If server handled this keybind.
 * @return false If server doesn't know this keybind, so wasn't handled.
 */
bool
bsi_keyboard_keybinds_process(struct bsi_input_device* device,
                              struct wlr_keyboard_key_event* event);

/**
 * @brief Appropriately handles the processed keybinds.
 *
 * @param server The server.
 * @param combo The pressed combo if recognized by the server.
 * @param syms The symbols state.
 * @param syms_len The symbols state len.
 * @return true If handled.
 * @return false If not handled.
 */
bool
bsi_keyboard_keybinds_handle(struct bsi_server* server,
                             enum bsi_keyboard_modifier combo,
                             const xkb_keysym_t* syms,
                             const size_t syms_len);

bool
bsi_keyboard_mod_none_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
bsi_keyboard_mod_ctrl_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
bsi_keyboard_mod_alt_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
bsi_keyboard_mod_super_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
bsi_keyboard_mod_ctrl_alt_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
bsi_keyboard_mod_ctrl_shift_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
bsi_keyboard_mod_ctrl_alt_shift_handle(struct bsi_server* server,
                                       xkb_keysym_t sym);

bool
bsi_keyboard_mod_super_shift_handle(struct bsi_server* server,
                                    xkb_keysym_t sym);
