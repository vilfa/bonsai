#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon.h>

#include "bonsai/config/input.h"
#include "bonsai/server.h"

// TODO: Something weird with software cursor, when not using the laptop
// touchpad, but using the connected mouse.

struct bsi_inputs*
bsi_inputs_init(struct bsi_inputs* bsi_inputs, struct bsi_server* bsi_server)
{
    assert(bsi_inputs);
    assert(bsi_server);
    assert(bsi_server->wlr_seat);

    bsi_inputs->bsi_server = bsi_server;
    bsi_inputs->wlr_seat = bsi_server->wlr_seat;
    bsi_inputs->len_pointers = 0;
    bsi_inputs->len_keyboards = 0;
    wl_list_init(&bsi_inputs->pointers);
    wl_list_init(&bsi_inputs->keyboards);

    return bsi_inputs;
}

void
bsi_inputs_pointer_add(struct bsi_inputs* bsi_inputs,
                       struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_inputs);
    assert(bsi_input_pointer);
    assert(bsi_input_pointer->wlr_cursor);
    assert(bsi_input_pointer->wlr_input_device);

    ++bsi_inputs->len_pointers;
    wl_list_insert(&bsi_inputs->pointers, &bsi_input_pointer->link);

    wlr_cursor_attach_input_device(bsi_input_pointer->wlr_cursor,
                                   bsi_input_pointer->wlr_input_device);
}

void
bsi_inputs_pointer_remove(struct bsi_inputs* bsi_inputs,
                          struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_inputs);
    assert(bsi_input_pointer);

    --bsi_inputs->len_pointers;
    wl_list_remove(&bsi_input_pointer->link);
    bsi_input_pointer_finish(bsi_input_pointer);
}

void
bsi_inputs_keyboard_add(struct bsi_inputs* bsi_inputs,
                        struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_inputs);
    assert(bsi_input_keyboard);
    assert(bsi_input_keyboard->wlr_input_device);

    ++bsi_inputs->len_keyboards;
    wl_list_insert(&bsi_inputs->keyboards, &bsi_input_keyboard->link);

    bsi_input_keyboard_keymap_set(bsi_input_keyboard,
                                  bsi_input_keyboard_rules,
                                  bsi_input_keyboard_rules_len);

    wlr_seat_set_keyboard(bsi_inputs->wlr_seat,
                          bsi_input_keyboard->wlr_input_device);
}

void
bsi_inputs_keyboard_remove(struct bsi_inputs* bsi_inputs,
                           struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_inputs);
    assert(bsi_input_keyboard);

    --bsi_inputs->len_keyboards;
    wl_list_remove(&bsi_input_keyboard->link);
    bsi_input_keyboard_finish(bsi_input_keyboard);
}

struct bsi_input_pointer*
bsi_input_pointer_init(struct bsi_input_pointer* bsi_input_pointer,
                       struct bsi_server* bsi_server,
                       struct wlr_input_device* wlr_input_device)
{
    assert(bsi_input_pointer);
    assert(bsi_server);
    assert(wlr_input_device);

    bsi_input_pointer->bsi_server = bsi_server;
    bsi_input_pointer->wlr_cursor = bsi_server->wlr_cursor;
    bsi_input_pointer->wlr_input_device = wlr_input_device;
    bsi_input_pointer->len_active_listen = 0;

    return bsi_input_pointer;
}

void
bsi_input_pointer_finish(struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_input_pointer);

    wl_list_remove(&bsi_input_pointer->listen.motion.link);
    wl_list_remove(&bsi_input_pointer->listen.motion_absolute.link);
    wl_list_remove(&bsi_input_pointer->listen.button.link);
    wl_list_remove(&bsi_input_pointer->listen.axis.link);
    wl_list_remove(&bsi_input_pointer->listen.frame.link);
    wl_list_remove(&bsi_input_pointer->listen.swipe_begin.link);
    wl_list_remove(&bsi_input_pointer->listen.swipe_update.link);
    wl_list_remove(&bsi_input_pointer->listen.swipe_end.link);
    wl_list_remove(&bsi_input_pointer->listen.pinch_begin.link);
    wl_list_remove(&bsi_input_pointer->listen.pinch_update.link);
    wl_list_remove(&bsi_input_pointer->listen.pinch_end.link);
    wl_list_remove(&bsi_input_pointer->listen.hold_begin.link);
    wl_list_remove(&bsi_input_pointer->listen.hold_end.link);
    bsi_input_pointer->len_active_listen = 0;
}

void
bsi_input_pointer_destroy(struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_input_pointer);

    free(bsi_input_pointer);
}

void
bsi_input_pointer_listener_add(struct bsi_input_pointer* bsi_input_pointer,
                               struct wl_listener* bsi_listener_memb,
                               struct wl_signal* bsi_signal_memb,
                               wl_notify_func_t func)
{
    assert(bsi_input_pointer);
    assert(func);

    bsi_listener_memb->notify = func;
    ++bsi_input_pointer->len_active_listen;
    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}

struct bsi_input_keyboard*
bsi_input_keyboard_init(struct bsi_input_keyboard* bsi_input_keyboard,
                        struct bsi_server* bsi_server,
                        struct wlr_input_device* wlr_input_device)
{
    assert(bsi_input_keyboard);
    assert(bsi_server);
    assert(wlr_input_device);

    bsi_input_keyboard->bsi_server = bsi_server;
    bsi_input_keyboard->wlr_input_device = wlr_input_device;
    bsi_input_keyboard->len_active_links = 0;

    return bsi_input_keyboard;
}

void
bsi_input_keyboard_finish(struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_input_keyboard);

    wl_list_remove(&bsi_input_keyboard->listen.key.link);
    wl_list_remove(&bsi_input_keyboard->listen.modifiers.link);
    wl_list_remove(&bsi_input_keyboard->listen.keymap.link);
    wl_list_remove(&bsi_input_keyboard->listen.repeat_info.link);
    wl_list_remove(&bsi_input_keyboard->listen.destroy.link);
    bsi_input_keyboard->len_active_links = 0;
}

void
bsi_input_keyboard_destroy(struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_input_keyboard);

    free(bsi_input_keyboard);
}

void
bsi_input_keyboard_keymap_set(struct bsi_input_keyboard* bsi_input_keyboard,
                              const struct xkb_rule_names* xkb_rule_names,
                              const size_t xkb_rule_names_len)
{
    assert(bsi_input_keyboard);

#define rs_len_max 50
    size_t rs_len[] = { [4] = 0 };
    char rules[rs_len_max] = { 0 }, models[rs_len_max] = { 0 },
         layouts[rs_len_max] = { 0 }, variants[rs_len_max] = { 0 },
         options[rs_len_max] = { 0 };

    for (size_t i = 0; i < xkb_rule_names_len; ++i) {
        const struct xkb_rule_names* rs = &xkb_rule_names[i];
        if (rs->rules != NULL &&
            rs_len[0] + strlen(rs->rules) < rs_len_max - 1) {
            if (rs_len[0] != 0)
                strcat(rules + strlen(rules), ",");
            strcat(rules + strlen(rules), rs->rules);
            rs_len[0] = strlen(rules);
        }
        if (rs->model != NULL &&
            rs_len[1] + strlen(rs->model) < rs_len_max - 1) {
            if (rs_len[1] != 0)
                strcat(models + strlen(models), ",");
            strcat(models + strlen(models), rs->model);
            rs_len[1] = strlen(models);
        }
        if (rs->layout != NULL &&
            rs_len[2] + strlen(rs->layout) < rs_len_max - 1) {
            if (rs_len[2] != 0)
                strcat(layouts + strlen(layouts), ",");
            strcat(layouts + strlen(layouts), rs->layout);
            rs_len[2] = strlen(layouts);
        }
        if (rs->variant != NULL &&
            rs_len[3] + strlen(rs->variant) < rs_len_max - 1) {
            if (rs_len[3] != 0)
                strcat(variants + strlen(variants), ",");
            strcat(variants + strlen(variants), rs->variant);
            rs_len[3] = strlen(variants);
        }
        if (rs->options != NULL && rs_len[4] == 0 &&
            rs_len[4] + strlen(rs->options) < rs_len_max - 1) {
            strcat(options + strlen(options), rs->options);
            rs_len[4] = strlen(options);
        }
    }
#undef rs_len_max

    struct xkb_rule_names xkb_rules_all = {
        .rules = rules,
        .model = models,
        .layout = layouts,
        .variant = variants,
        .options = options,
    };

    struct xkb_context* xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* xkb_keymap = xkb_keymap_new_from_names(
        xkb_context, &xkb_rules_all, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(bsi_input_keyboard->wlr_input_device->keyboard,
                            xkb_keymap);

    xkb_keymap_unref(xkb_keymap);
    xkb_context_unref(xkb_context);
}

void
bsi_input_keyboard_listener_add(struct bsi_input_keyboard* bsi_input_keyboard,
                                struct wl_listener* bsi_listener_memb,
                                struct wl_signal* bsi_signal_memb,
                                wl_notify_func_t func)
{
    assert(bsi_input_keyboard);
    assert(func);

    bsi_listener_memb->notify = func;
    ++bsi_input_keyboard->len_active_links;
    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}
