#include <assert.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon.h>

#include "bonsai/config/input.h"

struct bsi_inputs*
bsi_inputs_init(struct bsi_inputs* bsi_inputs, struct wlr_seat* wlr_seat)
{
    assert(bsi_inputs);
    assert(wlr_seat);

    bsi_inputs->wlr_seat = wlr_seat;
    bsi_inputs->len_pointers = 0;
    bsi_inputs->len_keyboards = 0;
    wl_list_init(&bsi_inputs->inputs_pointers);
    wl_list_init(&bsi_inputs->inputs_keyboards);

    return bsi_inputs;
}

void
bsi_inputs_add_pointer(struct bsi_inputs* bsi_inputs,
                       struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_inputs);
    assert(bsi_input_pointer);
    assert(bsi_input_pointer->wlr_cursor);
    assert(bsi_input_pointer->wlr_input_device);

    ++bsi_inputs->len_pointers;
    wl_list_insert(&bsi_inputs->inputs_pointers, &bsi_input_pointer->link);

    wlr_cursor_attach_input_device(bsi_input_pointer->wlr_cursor,
                                   bsi_input_pointer->wlr_input_device);
}

void
bsi_inputs_remove_pointer(struct bsi_inputs* bsi_inputs,
                          struct bsi_input_pointer* bsi_input_pointer)
{
    assert(bsi_inputs);
    assert(bsi_input_pointer);

    --bsi_inputs->len_pointers;
    wl_list_remove(&bsi_input_pointer->link);
    wl_list_remove(&bsi_input_pointer->cursor_motion.link);
    wl_list_remove(&bsi_input_pointer->cursor_motion_absolute.link);
    wl_list_remove(&bsi_input_pointer->cursor_button.link);
    wl_list_remove(&bsi_input_pointer->cursor_axis.link);
    wl_list_remove(&bsi_input_pointer->cursor_frame.link);
    free(bsi_input_pointer);
}

void
bsi_inputs_add_keyboard(struct bsi_inputs* bsi_inputs,
                        struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_inputs);
    assert(bsi_input_keyboard);
    assert(bsi_input_keyboard->wlr_input_device);

    ++bsi_inputs->len_keyboards;
    wl_list_insert(&bsi_inputs->inputs_keyboards, &bsi_input_keyboard->link);

    struct xkb_context* xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* xkb_keymap = xkb_keymap_new_from_names(
        xkb_context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(bsi_input_keyboard->wlr_input_device->keyboard,
                            xkb_keymap);

    xkb_keymap_unref(xkb_keymap);
    xkb_context_unref(xkb_context);

    wlr_seat_set_keyboard(bsi_inputs->wlr_seat,
                          bsi_input_keyboard->wlr_input_device);
}

void
bsi_inputs_remove_keyboard(struct bsi_inputs* bsi_inputs,
                           struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_inputs);
    assert(bsi_input_keyboard);

    --bsi_inputs->len_keyboards;
    wl_list_remove(&bsi_input_keyboard->link);
    wl_list_remove(&bsi_input_keyboard->key.link);
    wl_list_remove(&bsi_input_keyboard->modifier.link);
    free(bsi_input_keyboard);
}

size_t
bsi_inputs_len_pointers(struct bsi_inputs* bsi_inputs)
{
    assert(bsi_inputs);

    return bsi_inputs->len_pointers;
}

size_t
bsi_inputs_len_keyboard(struct bsi_inputs* bsi_inputs)
{
    assert(bsi_inputs);

    return bsi_inputs->len_keyboards;
}
