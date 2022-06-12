#pragma once

#include <wlr/types/wlr_keyboard.h>

#include "bonsai/input.h"

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

bool
keyboard_keybinds_process(struct bsi_input_device* device,
                          struct wlr_keyboard_key_event* event);

bool
keyboard_keybinds_handle(struct bsi_server* server,
                         enum bsi_keyboard_modifier combo,
                         const xkb_keysym_t* syms,
                         const size_t syms_len);

bool
keyboard_mod_none_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_ctrl_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_alt_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_super_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_ctrl_alt_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_ctrl_shift_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_ctrl_alt_shift_handle(struct bsi_server* server, xkb_keysym_t sym);

bool
keyboard_mod_super_shift_handle(struct bsi_server* server, xkb_keysym_t sym);
