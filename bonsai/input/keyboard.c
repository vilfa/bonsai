/* Most keyboard stuff is already in config/input.c, but that is mostly about
 * configuring a new keyboard if it is attached. This file specifically holds
 * helpers for events that happen at server runtime, related to a keyboard. */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "bonsai/desktop/view.h"
#include "bonsai/input.h"
#include "bonsai/input/keyboard.h"
#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

bool
bsi_keyboard_keybinds_process(struct bsi_input_device* device,
                              struct wlr_keyboard_key_event* event)
{
    assert(device->type == BSI_INPUT_DEVICE_KEYBOARD);

    struct bsi_server* server = device->server;
    struct wlr_keyboard* wlr_keyboard = device->device->keyboard;

    /* Translate libinput -> xkbcommon keycode. */
    uint32_t keycode = event->keycode + 8;
    uint32_t mods = wlr_keyboard_get_modifiers(wlr_keyboard);

    /* Get keysyms based on keyboard keymap. */
    const xkb_keysym_t* syms;
    const size_t syms_len =
        xkb_state_key_get_syms(wlr_keyboard->xkb_state, keycode, &syms);

    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED && mods != 0) {
        uint32_t combo;

        if ((mods ^
             (WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT | WLR_MODIFIER_SHIFT)) == 0)
            combo = BSI_KEYBOARD_MOD_CTRL_ALT_SHIFT;
        else if ((mods ^ (WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT)) == 0)
            combo = BSI_KEYBOARD_MOD_CTRL_ALT;
        else if ((mods ^ (WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT)) == 0)
            combo = BSI_KEYBOARD_MOD_CTRL_SHIFT;
        else if ((mods ^ (WLR_MODIFIER_LOGO | WLR_MODIFIER_SHIFT)) == 0)
            combo = BSI_KEYBOARD_MOD_SUPER_SHIFT;
        else if (mods & WLR_MODIFIER_CTRL)
            combo = BSI_KEYBOARD_MOD_CTRL;
        else if (mods & WLR_MODIFIER_ALT)
            combo = BSI_KEYBOARD_MOD_ALT;
        else if (mods & WLR_MODIFIER_LOGO)
            combo = BSI_KEYBOARD_MOD_SUPER;
        else
            return false;

        return bsi_keyboard_keybinds_handle(server, combo, syms, syms_len);
    }

    return false;
}

bool
bsi_keyboard_keybinds_handle(struct bsi_server* server,
                             enum bsi_keyboard_modifier combo,
                             const xkb_keysym_t* syms,
                             const size_t syms_len)
{
    bool handled = false;
    switch (combo) {
        case BSI_KEYBOARD_MOD_CTRL:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_ctrl_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_ALT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_alt_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_SUPER:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_super_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_ALT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_ctrl_alt_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_SHIFT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_ctrl_shift_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_ALT_SHIFT:
            for (size_t i = 0; i < syms_len; ++i)
                handled =
                    bsi_keyboard_mod_ctrl_alt_shift_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_SUPER_SHIFT:
            for (size_t i = 0; i < syms_len; i++)
                handled = bsi_keyboard_mod_super_shift_handle(server, syms[i]);
            break;
    }
    return handled;
}

bool
bsi_keyboard_mod_ctrl_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    bsi_debug("Got Ctrl mod");
    return false;
}

bool
bsi_keyboard_mod_alt_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    bsi_debug("Got Alt mod");
    switch (sym) {
        case XKB_KEY_Tab: {
            bsi_debug("Got Alt+Tab -> cycle views");
            bsi_views_mru_focus(server);
            return true;
        }
    }
    return false;
}

bool
bsi_keyboard_mod_super_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    bsi_debug("Got Super mod");
    switch (sym) {
        case XKB_KEY_d: {
            bsi_debug("Got Super+d -> bemenu");
            char* const argp[] = { "bemenu-run", "-c", "-i", NULL };
            return bsi_util_tryexec(argp, 4);
        }
        case XKB_KEY_Return: {
            bsi_debug("Got Super+Return -> term");
            char* const argp[] = { "foot", NULL };
            return bsi_util_tryexec(argp, 2);
        }
        case XKB_KEY_Up: {
            bsi_debug("Got Super+Up -> maximize active");
            struct bsi_view* focused = bsi_views_get_focused(server);
            if (focused)
                bsi_view_set_maximized(focused, true);
            return true;
        }
        case XKB_KEY_Down: {
            bsi_debug("Got Super+Down -> unmaximize active");
            struct bsi_view* focused = bsi_views_get_focused(server);
            if (focused)
                bsi_view_set_maximized(focused, false);
            return true;
        }
        case XKB_KEY_Left: {
            bsi_debug("Got Super+Left -> tile left/untile");
            struct bsi_view* focused = bsi_views_get_focused(server);
            if (focused) {
                switch (focused->state) {
                    case BSI_VIEW_STATE_TILED_RIGHT:
                        bsi_view_set_tiled_right(focused, false);
                        break;
                    case BSI_VIEW_STATE_NORMAL:
                        bsi_view_set_tiled_left(focused, true);
                        break;
                    default:
                        break;
                }
            }
            return true;
        }
        case XKB_KEY_Right: {
            bsi_debug("Got Super+Right -> tile right/untile");
            struct bsi_view* focused = bsi_views_get_focused(server);
            if (focused) {
                switch (focused->state) {
                    case BSI_VIEW_STATE_TILED_LEFT:
                        bsi_view_set_tiled_left(focused, false);
                        break;
                    case BSI_VIEW_STATE_NORMAL:
                        bsi_view_set_tiled_right(focused, true);
                        break;
                    default:
                        break;
                }
            }
            return true;
        }
    }
    return false;
}

bool
bsi_keyboard_mod_ctrl_alt_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    bsi_debug("Got Ctrl+Alt mod");
    return false;
}

bool
bsi_keyboard_mod_ctrl_shift_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    bsi_debug("Got Ctrl+Shift mod");
    return false;
}

bool
bsi_keyboard_mod_ctrl_alt_shift_handle(struct bsi_server* server,
                                       xkb_keysym_t sym)
{
    bsi_debug("Got Ctrl+Alt+Shift mod");
    return false;
}

bool
bsi_keyboard_mod_super_shift_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    bsi_debug("Got Super+Shift mod");
    switch (sym) {
        case XKB_KEY_Q:
            bsi_info("Got Super+Shift+Q -> exit");
            bsi_server_exit(server);
            break;
    }
    return false;
}
