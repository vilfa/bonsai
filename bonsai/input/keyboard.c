/* Most keyboard stuff is already in config/input.c, but that is mostly about
 * configuring a new keyboard if it is attached. This file specifically holds
 * helpers for events that happen at server runtime, related to a keyboard. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-server-protocol.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "bonsai/config/input.h"
#include "bonsai/input/keyboard.h"
#include "bonsai/server.h"

#define GIMME_ALL_KEYBINDINGS

bool
bsi_keyboard_keybinds_process(struct bsi_input_keyboard* bsi_input_keyboard,
                              struct wlr_event_keyboard_key* event)
{
    struct bsi_server* bsi_server = bsi_input_keyboard->bsi_server;
    struct wlr_keyboard* keyboard =
        bsi_input_keyboard->wlr_input_device->keyboard;

    /* Translate libinput -> xkbcommon keycode. */
    uint32_t keycode = event->keycode + 8;
    uint32_t mods = wlr_keyboard_get_modifiers(keyboard);

    /* Get keysyms based on keyboard keymap. */
    const xkb_keysym_t* syms;
    const size_t syms_len =
        xkb_state_key_get_syms(keyboard->xkb_state, keycode, &syms);

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

        return bsi_keyboard_keybinds_handle(bsi_server, combo, syms, syms_len);
    }

    return false;
}

bool
bsi_keyboard_keybinds_handle(struct bsi_server* bsi_server,
                             enum bsi_keyboard_modifier combo,
                             const xkb_keysym_t* syms,
                             const size_t syms_len)
{
    bool handled = false;
    switch (combo) {
        case BSI_KEYBOARD_MOD_CTRL:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_ctrl_handle(bsi_server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_ALT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_alt_handle(bsi_server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_SUPER:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_super_handle(bsi_server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_ALT:
            for (size_t i = 0; i < syms_len; ++i)
                handled = bsi_keyboard_mod_ctrl_alt_handle(bsi_server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_SHIFT:
            for (size_t i = 0; i < syms_len; ++i)
                handled =
                    bsi_keyboard_mod_ctrl_shift_handle(bsi_server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_CTRL_ALT_SHIFT:
            for (size_t i = 0; i < syms_len; ++i)
                handled =
                    bsi_keyboard_mod_ctrl_alt_shift_handle(bsi_server, syms[i]);
            break;
        case BSI_KEYBOARD_MOD_SUPER_SHIFT:
            for (size_t i = 0; i < syms_len; i++)
                handled =
                    bsi_keyboard_mod_super_shift_handle(bsi_server, syms[i]);
            break;
    }
    return handled;
}

bool
bsi_keyboard_mod_ctrl_handle(struct bsi_server* bsi_server, xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Ctrl mod");
#endif
    return false;
}

bool
bsi_keyboard_mod_alt_handle(struct bsi_server* bsi_server, xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Alt mod");
#endif
    return false;
}

bool
bsi_keyboard_mod_super_handle(struct bsi_server* bsi_server, xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Super mod");
#endif
    switch (sym) {
        case XKB_KEY_d:
            // TODO: Make a helper for this stuff.
            switch (fork()) {
                case 0: {
                    wlr_log(WLR_INFO, "Got Super+d -> open menu");

                    char path[255] = { 0 }, sock[255] = { 0 },
                         rdir[255] = { 0 };
                    sprintf(path, "PATH=%s", getenv("PATH"));
                    sprintf(sock, "WAYLAND_DISPLAY=%s", bsi_server->wl_socket);
                    sprintf(
                        rdir, "XDG_RUNTIME_DIR=%s", getenv("XDG_RUNTIME_DIR"));

                    char* const argp[] = {
                        "/usr/bin/bemenu-run", "-c", "-i", NULL
                    };
                    char* const envp[] = { path, sock, rdir, NULL };
                    execve(argp[0], argp, envp);

                    wlr_log(WLR_ERROR, "Exec failed: %s", strerror(errno));
                    _exit(EXIT_FAILURE);
                    break;
                }
                case -1:
                    wlr_log(WLR_ERROR, "Fork failed: %s", strerror(errno));
                    return false;
                default:
                    return true;
            }
            break;
        case XKB_KEY_Return:
            switch (fork()) {
                case 0: {
                    wlr_log(WLR_INFO, "Got Super+Return -> open terminal");

                    char path[255] = { 0 }, sock[255] = { 0 },
                         rdir[255] = { 0 };
                    sprintf(path, "PATH=%s", getenv("PATH"));
                    sprintf(sock, "WAYLAND_DISPLAY=%s", bsi_server->wl_socket);
                    sprintf(
                        rdir, "XDG_RUNTIME_DIR=%s", getenv("XDG_RUNTIME_DIR"));

                    char* const argp[] = { "/usr/bin/foot", NULL };
                    char* const envp[] = { path, sock, rdir, NULL };
                    execve(argp[0], argp, envp);

                    wlr_log(WLR_ERROR, "Exec failed: %s", strerror(errno));
                    _exit(EXIT_FAILURE);
                    break;
                }
                case -1:
                    wlr_log(WLR_ERROR, "Fork failed: %s", strerror(errno));
                    return false;
                default:
                    return true;
            }
            break;
    }
    return false;
}

bool
bsi_keyboard_mod_ctrl_alt_handle(struct bsi_server* bsi_server,
                                 xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Ctrl+Alt mod");
#endif
    return false;
}

bool
bsi_keyboard_mod_ctrl_shift_handle(struct bsi_server* bsi_server,
                                   xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Ctrl+Shift mod");
#endif
    return false;
}

bool
bsi_keyboard_mod_ctrl_alt_shift_handle(struct bsi_server* bsi_server,
                                       xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Ctrl+Alt+Shift mod");
#endif
    return false;
}

bool
bsi_keyboard_mod_super_shift_handle(struct bsi_server* bsi_server,
                                    xkb_keysym_t sym)
{
#ifdef GIMME_ALL_KEYBINDINGS
    wlr_log(WLR_DEBUG, "Got Super+Shift mod");
#endif
    switch (sym) {
        case XKB_KEY_Q:
            wlr_log(WLR_INFO, "Got Super+Shift+Q -> exit");
            bsi_server_exit(bsi_server);
            break;
    }
    return false;
}
