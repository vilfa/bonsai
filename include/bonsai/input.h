#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

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
