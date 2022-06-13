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
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "bonsai/config/config.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/input.h"
#include "bonsai/input/keyboard.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

bool
keyboard_keybinds_process(struct bsi_input_device* device,
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
    const bool mod_none = (syms_len == 1 && (syms[0] == XKB_KEY_Print ||
                                             syms[0] == XKB_KEY_Escape ||
                                             syms[0] == XKB_KEY_F11));

    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED &&
        (mods != 0 || mod_none)) {
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
        else if (mod_none)
            combo = BSI_KEYBOARD_MOD_NONE;
        else
            return false;

        return keyboard_keybinds_handle(server, combo, syms, syms_len);
    }

    return false;
}

bool
keyboard_keybinds_handle(struct bsi_server* server,
                         enum bsi_keyboard_modifier combo,
                         const xkb_keysym_t* syms,
                         const size_t syms_len)
{
    bool handled = false;
    switch (combo) {
        case BSI_KEYBOARD_MOD_NONE:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_none_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_ctrl_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_ALT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_alt_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_SUPER:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_super_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_ALT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_ctrl_alt_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_SHIFT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_ctrl_shift_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_ALT_SHIFT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = keyboard_mod_ctrl_alt_shift_handle(server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_SUPER_SHIFT:
            for (size_t i = 0; i < syms_len; i++)
                handled = keyboard_mod_super_shift_handle(server, syms[i]);
            break;
    }
    return handled;
}

bool
keyboard_mod_none_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    switch (sym) {
        case XKB_KEY_Print: {
            debug("Got Print -> screenshot all outputs");
            char* const argp[] = { "sh", "-c", "grim - | wl-copy", NULL };
            return util_tryexec(argp, 4);
        }
        case XKB_KEY_Escape: {
            debug("Got Escape -> un-fullscreen if fulscreen");
            struct bsi_view* focused = views_get_focused(server);
            if (focused && focused->state == BSI_VIEW_STATE_FULLSCREEN) {
                view_set_fullscreen(focused, false);
                return true;
            }
            return false;
        }
        case XKB_KEY_F11: {
            debug("Got F11 -> fullscreen focused view");
            struct bsi_view* focused = views_get_focused(server);
            if (focused && focused->state != BSI_VIEW_STATE_FULLSCREEN) {
                view_set_fullscreen(focused, true);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool
keyboard_mod_ctrl_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Ctrl mod");
    switch (sym) {
        case XKB_KEY_Print: {
            debug("Got Ctrl+Print -> screenshot active output");
            struct wlr_output* output =
                wlr_output_layout_output_at(server->wlr_output_layout,
                                            server->wlr_cursor->x,
                                            server->wlr_cursor->y);

            if (!output)
                return true;

            char cmd[50] = { 0 };
            sprintf(cmd, "grim -o %s - | wl-copy", output->name);
            char* const argp[] = { "sh", "-c", cmd, NULL };
            return util_tryexec(argp, 4);
        }
    }
    return false;
}

bool
keyboard_mod_alt_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Alt mod");
    switch (sym) {
        case XKB_KEY_Tab: {
            debug("Got Alt+Tab -> cycle views");
            views_focus_recent(server);
            return true;
        }
    }
    return false;
}

bool
keyboard_mod_super_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Super mod");
    switch (sym) {
        case XKB_KEY_l:
        case XKB_KEY_L: {
            debug("Got Super+L -> lock session");
            char* const argp[] = { "swaylock", NULL };
            return util_tryexec(argp, 2);
        }
        case XKB_KEY_d:
        case XKB_KEY_D: {
            debug("Got Super+D -> bemenu");
            char* const argp[] = { "bemenu-run", "-c", "-i", NULL };
            return util_tryexec(argp, 4);
        }
        case XKB_KEY_Return: {
            debug("Got Super+Return -> term");
            char* const argp[] = { "foot", NULL };
            return util_tryexec(argp, 2);
        }
        case XKB_KEY_Up: {
            debug("Got Super+Up -> maximize active");
            struct bsi_view* focused = views_get_focused(server);
            if (focused)
                view_set_maximized(focused, true);
            return true;
        }
        case XKB_KEY_Down: {
            debug("Got Super+Down -> unmaximize active");
            struct bsi_view* focused = views_get_focused(server);
            if (focused)
                view_set_maximized(focused, false);
            return true;
        }
        case XKB_KEY_Left: {
            debug("Got Super+Left -> tile left/untile");
            struct bsi_view* focused = views_get_focused(server);
            if (focused) {
                switch (focused->state) {
                    case BSI_VIEW_STATE_TILED_RIGHT:
                        view_set_tiled_right(focused, false);
                        break;
                    case BSI_VIEW_STATE_NORMAL:
                        view_set_tiled_left(focused, true);
                        break;
                    default:
                        break;
                }
            }
            return true;
        }
        case XKB_KEY_Right: {
            debug("Got Super+Right -> tile right/untile");
            struct bsi_view* focused = views_get_focused(server);
            if (focused) {
                switch (focused->state) {
                    case BSI_VIEW_STATE_TILED_LEFT:
                        view_set_tiled_left(focused, false);
                        break;
                    case BSI_VIEW_STATE_NORMAL:
                        view_set_tiled_right(focused, true);
                        break;
                    default:
                        break;
                }
            }
            return true;
        }
        case XKB_KEY_x:
        case XKB_KEY_X: {
            debug("Got Super+X -> hide all workspace views");
            workspace_views_show_all(server->active_workspace, false);
            return true;
        }
        case XKB_KEY_z:
        case XKB_KEY_Z: {
            debug("Got Super+Z -> show all workspace views");
            workspace_views_show_all(server->active_workspace, true);
            return true;
        }
    }
    return false;
}

bool
keyboard_mod_ctrl_alt_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Ctrl+Alt mod");
    switch (sym) {
        case XKB_KEY_Right: {
            struct wlr_output* output =
                wlr_output_layout_output_at(server->wlr_output_layout,
                                            server->wlr_cursor->x,
                                            server->wlr_cursor->y);

            if (!output)
                return true;

            struct bsi_output* active_output = outputs_find(server, output);
            workspaces_next(active_output);
            return true;
        }
        case XKB_KEY_Left: {
            struct wlr_output* output =
                wlr_output_layout_output_at(server->wlr_output_layout,
                                            server->wlr_cursor->x,
                                            server->wlr_cursor->y);

            if (!output)
                return true;

            struct bsi_output* active_output = outputs_find(server, output);
            workspaces_prev(active_output);
            return true;
        }
    }
    return false;
}

bool
keyboard_mod_ctrl_shift_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Ctrl+Shift mod");
    switch (sym) {
        case XKB_KEY_Print: {
            debug("Got Ctrl+Shift+Print -> screenshot selection");
            char* const argp[] = {
                "sh", "-c", "grim -g \"$(slurp)\" - | wl-copy", NULL
            };
            return util_tryexec(argp, 4);
        }
    }
    return false;
}

bool
keyboard_mod_ctrl_alt_shift_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Ctrl+Alt+Shift mod");
    return false;
}

bool
keyboard_mod_super_shift_handle(struct bsi_server* server, xkb_keysym_t sym)
{
    debug("Got Super+Shift mod");
    switch (sym) {
        case XKB_KEY_q:
        case XKB_KEY_Q:
            info("Got Super+Shift+Q -> exit");
            wlr_backend_destroy(server->wlr_backend);
            config_destroy(server->config.config);
            server_destroy(server);
            exit(EXIT_SUCCESS);
    }
    return false;
}
