#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>
#include <xkbcommon/xkbcommon.h>

struct bsi_input_manager
{
    struct bsi_server* server;

    struct wl_list devices;
    struct wl_list seats;

    struct wlr_input_inhibit_manager* input_inhibit;
    struct wlr_keyboard_shortcuts_inhibit_manager_v1*
        keyboard_shortcuts_inhibit;
    struct wlr_virtual_keyboard_manager_v1* virtual_keyboard;
    struct wlr_virtual_pointer_manager_v1* virtual_pointer;

    struct
    {
        /* wlr_backend */
        struct wl_listener new_input;
        /* wlr_input_inhibit_manager */
        struct wl_listener input_inhibit_activate;
        struct wl_listener input_inhibit_deactivate;
        /* wlr_keyboard_shortcuts_inhibit_manager_v1 */
        struct wl_listener new_keyboard_shortcuts_inhibitor;
        /* wlr_virtual_keyboard_manager_v1 */
        struct wl_listener new_virtual_keyboard;
        /* wlr_virtual_pointer_manager_v1 */
        struct wl_listener new_virtual_pointer;
    } listen;
};

enum bsi_input_device_type
{
    BSI_INPUT_DEVICE_POINTER,
    BSI_INPUT_DEVICE_KEYBOARD,
};

struct bsi_input_device
{
    struct bsi_server* server;
    struct wlr_cursor* cursor;
    struct wlr_input_device* device;

    enum bsi_input_device_type type;

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
        /* wlr_input_device::keyboard */
        struct wl_listener key;
        struct wl_listener modifiers;
        /* wlr_input_device::destroy */
        struct wl_listener destroy;
    } listen;

    struct wl_list link_server; // bsi_server
};

/* bsi_input_manager */
struct bsi_input_manager*
input_manager_init(struct bsi_server* server);

void
input_manager_destroy(struct bsi_input_manager* input_manager);

/* bsi_input_device */
struct bsi_input_device*
input_device_init(struct bsi_input_device* input_device,
                  enum bsi_input_device_type type,
                  struct bsi_server* server,
                  struct wlr_input_device* device);

void
input_device_destroy(struct bsi_input_device* input_device);

void
input_device_keymap_set(struct bsi_input_device* input_device,
                        const struct xkb_rule_names* xkb_rule_names);
