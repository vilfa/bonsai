#include <assert.h>
#include <stdlib.h>
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
bsi_inputs_init(struct bsi_inputs* bsi_inputs, struct wlr_seat* wlr_seat)
{
    assert(bsi_inputs);
    assert(wlr_seat);

    bsi_inputs->wlr_seat = wlr_seat;
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
    bsi_input_pointer_listeners_unlink_all(bsi_input_pointer);
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

    bsi_input_keyboard_keymap_set(bsi_input_keyboard, NULL);

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
    bsi_input_keyboard_listeners_unlink_all(bsi_input_keyboard);
}

size_t
bsi_inputs_len_pointers(struct bsi_inputs* bsi_inputs)
{
    assert(bsi_inputs);

    return bsi_inputs->len_pointers;
}

size_t
bsi_inputs_len_keyboards(struct bsi_inputs* bsi_inputs)
{
    assert(bsi_inputs);

    return bsi_inputs->len_keyboards;
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
    bsi_input_pointer->active_listeners = 0;
    bsi_input_pointer->len_active_links = 0;

    return bsi_input_pointer;
}

void
bsi_input_pointer_destroy(struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_input_pointer);

    free(bsi_input_pointer);
}

void
bsi_input_pointer_listener_add(
    struct bsi_input_pointer* bsi_input_pointer,
    enum bsi_input_pointer_listener_mask bsi_listener_type,
    struct wl_listener* bsi_listener_memb,
    struct wl_signal* bsi_signal_memb,
    wl_notify_func_t func)
{
    assert(bsi_input_pointer);
    assert(func);

    bsi_listener_memb->notify = func;
    bsi_input_pointer->active_listeners |= bsi_listener_type;
    bsi_input_pointer->active_links[bsi_input_pointer->len_active_links++] =
        &bsi_listener_memb->link;

    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}

void
bsi_input_pointer_listeners_unlink_all(
    struct bsi_input_pointer* bsi_input_pointer)
{
    for (size_t i = 0; i < bsi_input_pointer->len_active_links; ++i) {
        if (bsi_input_pointer->active_links[i] != NULL)
            wl_list_remove(bsi_input_pointer->active_links[i]);
    }
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
    bsi_input_keyboard->active_listeners = 0;
    bsi_input_keyboard->len_active_links = 0;

    return bsi_input_keyboard;
}

void
bsi_input_keyboard_keymap_set(struct bsi_input_keyboard* bsi_input_keyboard,
                              const struct xkb_rule_names* xkb_rule_names)
{
    assert(bsi_input_keyboard);

    struct xkb_context* xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* xkb_keymap = xkb_keymap_new_from_names(
        xkb_context, xkb_rule_names, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(bsi_input_keyboard->wlr_input_device->keyboard,
                            xkb_keymap);

    xkb_keymap_unref(xkb_keymap);
    xkb_context_unref(xkb_context);
}

void
bsi_input_keyboard_layout_set(struct bsi_input_keyboard* bsi_input_keyboard,
                              const char* layout)
{
    assert(bsi_input_keyboard);

    // TODO: Wat do?
}

void
bsi_input_keyboard_destroy(struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_input_keyboard);

    free(bsi_input_keyboard);
}

void
bsi_input_keyboard_listener_add(
    struct bsi_input_keyboard* bsi_input_keyboard,
    enum bsi_input_keyboard_listener_mask bsi_listener_type,
    struct wl_listener* bsi_listener_memb,
    struct wl_signal* bsi_signal_memb,
    wl_notify_func_t func)
{
    assert(bsi_input_keyboard);
    assert(func);

    bsi_listener_memb->notify = func;
    bsi_input_keyboard->active_listeners |= bsi_listener_type;
    bsi_input_keyboard->active_links[bsi_input_keyboard->len_active_links++] =
        &bsi_listener_memb->link;

    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}

void
bsi_input_keyboard_listeners_unlink_all(
    struct bsi_input_keyboard* bsi_input_keyboard)
{
    for (size_t i = 0; i < bsi_input_keyboard->len_active_links; ++i) {
        if (bsi_input_keyboard->active_links[i] != NULL)
            wl_list_remove(bsi_input_keyboard->active_links[i]);
    }
}
