#include "bonsai/config/input.h"
#include <assert.h>
#include <stdlib.h>
#include <wayland-util.h>

struct bsi_inputs*
bsi_inputs_init(struct bsi_inputs* bsi_inputs, struct wlr_seat* wlr_seat)
{
    assert(bsi_inputs);
    assert(wlr_seat);

    bsi_inputs->seat = wlr_seat;
    bsi_inputs->len_cursors = 0;
    bsi_inputs->len_keyboards = 0;
    wl_list_init(&bsi_inputs->inputs_cursors);
    wl_list_init(&bsi_inputs->inputs_keyboards);

    return bsi_inputs;
}

void
bsi_inputs_add_cursor(struct bsi_inputs* bsi_inputs,
                      struct bsi_input_cursor* bsi_input_cursor)
{
    assert(bsi_inputs);
    assert(bsi_input_cursor);

    ++bsi_inputs->len_cursors;
    wl_list_insert(&bsi_inputs->inputs_cursors, &bsi_input_cursor->link);
}

void
bsi_inputs_remove_cursor(struct bsi_inputs* bsi_inputs,
                         struct bsi_input_cursor* bsi_input_cursor)
{
    assert(bsi_inputs);
    assert(bsi_input_cursor);

    --bsi_inputs->len_cursors;
    wl_list_remove(&bsi_input_cursor->link);
    wl_list_remove(&bsi_input_cursor->cursor_motion.link);
    wl_list_remove(&bsi_input_cursor->cursor_motion_absolute.link);
    wl_list_remove(&bsi_input_cursor->cursor_button.link);
    wl_list_remove(&bsi_input_cursor->cursor_axis.link);
    wl_list_remove(&bsi_input_cursor->cursor_frame.link);
    free(bsi_input_cursor);
}

void
bsi_inputs_add_keyboard(struct bsi_inputs* bsi_inputs,
                        struct bsi_input_keyboard* bsi_input_keyboard)
{
    assert(bsi_inputs);
    assert(bsi_input_keyboard);

    ++bsi_inputs->len_keyboards;
    wl_list_insert(&bsi_inputs->inputs_keyboards, &bsi_input_keyboard->link);
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
bsi_inputs_len_cursors(struct bsi_inputs* bsi_inputs)
{
    assert(bsi_inputs);

    return bsi_inputs->len_cursors;
}

size_t
bsi_inputs_len_keyboard(struct bsi_inputs* bsi_inputs)
{
    assert(bsi_inputs);

    return bsi_inputs->len_keyboards;
}
