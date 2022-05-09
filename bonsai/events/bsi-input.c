/**
 * @file bsi-input.c
 * @brief Holds all event listeners for `bsi_input_{pointer,keyboard}`.
 *
 *
 */

#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"

void
bsi_input_pointer_motion_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event motion from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_motion_absolute_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event motion_absolute from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_button_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event button from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_axis_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event axis from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_frame_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event frame from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_swipe_begin_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event swipe_begin from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_swipe_update_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event swipe_update from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_swipe_end_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event swipe_end from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_pinch_begin_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event pinch_begin from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_pinch_update_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event pinch_update from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_pinch_end_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event pinch_end from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_hold_begin_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event hold_begin from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_pointer_hold_end_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event hold_end from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_keyboard_key_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event key from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_keyboard_modifiers_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event modifiers from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_keyboard_keymap_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event keymap from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_keyboard_repeat_info_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event repeat_info from wlr_input_device");
#warning "Not implemented"
}

void
bsi_input_keyboard_destroy_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_input_device");
#warning "Not implemented"
}
