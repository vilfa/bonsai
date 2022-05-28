/**
 * @file bsi-input.c
 * @brief Holds all event handlers for `bsi_input_{pointer,keyboard}`.
 *
 *
 */

#include <assert.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>

#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/input/keyboard.h"
#include "bonsai/server.h"

#define GIMME_ALL_POINTER_EVENTS
#define GIMME_ALL_KEYBOARD_EVENTS

void
bsi_input_pointer_motion_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event motion from wlr_input_device");
#endif

    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.motion);
    struct bsi_server* server = pointer->server;
    struct wlr_pointer_motion_event* event = data;
    union bsi_cursor_event bsi_cursor_event = { .motion = event };

    /* Firstly, move the cursor. */
    wlr_cursor_move(
        pointer->cursor, pointer->device, event->delta_x, event->delta_y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(server, bsi_cursor_event);
}

void
bsi_input_pointer_motion_absolute_notify(struct wl_listener* listener,
                                         void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event motion_absolute from wlr_input_device");
#endif

    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.motion_absolute);
    struct bsi_server* server = pointer->server;
    struct wlr_pointer_motion_absolute_event* event = data;
    union bsi_cursor_event bsi_cursor_event = { .motion_absolute = event };

    /* Firstly, warp the absolute motion to our output layout constraints. */
    wlr_cursor_warp_absolute(
        server->wlr_cursor, pointer->device, event->x, event->y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(server, bsi_cursor_event);
}

void
bsi_input_pointer_button_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event button from wlr_input_device");
#endif

    /* This event is forwarded by wlr_cursor, when a pointer emits a button
     * event. */

    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.button);
    struct bsi_server* server = pointer->server;
    struct wlr_seat* wlr_seat = pointer->server->wlr_seat;
    struct wlr_pointer_button_event* event = data;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_button(
        wlr_seat, event->time_msec, event->button, event->state);

    /* Get the view under pointer, its surface, and surface relative pointer
     * coordinates. */
    double sx, sy;
    struct wlr_surface* surface = NULL;
    struct bsi_view* view = bsi_cursor_view_at(server, &surface, &sx, &sy);

    switch (event->state) {
        case WLR_BUTTON_RELEASED:
            /* Exit interactive mode. */
            server->cursor.cursor_mode = BSI_CURSOR_NORMAL;
            bsi_cursor_image_set(server, BSI_CURSOR_IMAGE_NORMAL);
            break;
        case WLR_BUTTON_PRESSED:
            /* Focus a client that was clicked. */
            if (view != NULL)
                bsi_view_focus(view);
            break;
    }
}

void
bsi_input_pointer_axis_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event axis from wlr_input_device");
#endif

    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.axis);
    struct wlr_seat* wlr_seat = pointer->server->wlr_seat;
    struct wlr_pointer_axis_event* event = data;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_axis(wlr_seat,
                                 event->time_msec,
                                 event->orientation,
                                 event->delta,
                                 event->delta_discrete,
                                 event->source);
}

void
bsi_input_pointer_frame_notify(struct wl_listener* listener,
                               __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event frame from wlr_input_device");
#endif

    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.frame);
    struct wlr_seat* wlr_seat = pointer->server->wlr_seat;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_frame(wlr_seat);
}

void
bsi_input_pointer_swipe_begin_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event swipe_begin from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.swipe_begin);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_swipe_begin_event* event = data;

    // TODO: Server is primary handler.
}

void
bsi_input_pointer_swipe_update_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event swipe_update from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.swipe_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_swipe_update_event* event = data;

    // TODO: Server is primary handler.
}

void
bsi_input_pointer_swipe_end_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event swipe_end from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.swipe_end);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_swipe_end_event* event = data;

    // TODO: Server is primary handler.
}

void
bsi_input_pointer_pinch_begin_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event pinch_begin from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_begin);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_pinch_begin_event* event = data;
}

void
bsi_input_pointer_pinch_update_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event pinch_update from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_pinch_update_event* event = data;
}

void
bsi_input_pointer_pinch_end_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event pinch_end from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_pinch_end_event* event = data;
}

void
bsi_input_pointer_hold_begin_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event hold_begin from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_hold_begin_event* event = data;
}

void
bsi_input_pointer_hold_end_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event hold_end from wlr_input_device");
#endif

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_hold_end_event* event = data;
}

void
bsi_input_keyboard_key_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event key from wlr_input_device");
#endif

    struct bsi_input_keyboard* keyboard =
        wl_container_of(listener, keyboard, listen.key);
    struct wlr_seat* wlr_seat = keyboard->server->wlr_seat;
    struct wlr_keyboard_key_event* event = data;

    if (!bsi_keyboard_keybinds_process(keyboard, event)) {
        /* A seat can only have one keyboard, but multiple keyboards can be
         * attached. Switch the seat to the correct underlying keyboard. Roots
         * handles this for us :). */
        wlr_seat_set_keyboard(wlr_seat, keyboard->device->keyboard);
        /* The server knows not thy keybind, the client shall handle it. Notify
         * client of keys not handled by the server. */
        wlr_seat_keyboard_notify_key(
            wlr_seat, event->time_msec, event->keycode, event->state);
    }
}

void
bsi_input_keyboard_modifiers_notify(struct wl_listener* listener,
                                    __attribute__((unused)) void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event modifiers from wlr_input_device");
#endif

    struct bsi_input_keyboard* keyboard =
        wl_container_of(listener, keyboard, listen.modifiers);
    struct wlr_seat* wlr_seat = keyboard->server->wlr_seat;
    struct wlr_input_device* device = keyboard->device;

    /* A seat can only have one keyboard, but multiple keyboards can be
     * attached. Switch the seat to the correct underlying keyboard. Roots
     * handles this for us :). */
    wlr_seat_set_keyboard(wlr_seat, device->keyboard);
    /* Notify client of keys not handled by the server. */
    wlr_seat_keyboard_notify_modifiers(wlr_seat, &device->keyboard->modifiers);
}

void
bsi_input_device_destroy_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_input_device");
#endif

    struct wlr_input_device* device = data;
    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD: {
            struct bsi_input_keyboard* keyboard =
                wl_container_of(listener, keyboard, listen.destroy);
            struct bsi_server* server = keyboard->server;
            bsi_inputs_keyboard_remove(server, keyboard);
            bsi_input_keyboard_finish(keyboard);
            bsi_input_keyboard_destroy(keyboard);
            break;
        }
        case WLR_INPUT_DEVICE_POINTER: {
            struct bsi_input_pointer* pointer =
                wl_container_of(listener, pointer, listen.destroy);
            struct bsi_server* server = pointer->server;
            bsi_inputs_pointer_remove(server, pointer);
            bsi_input_pointer_finish(pointer);
            bsi_input_pointer_destroy(pointer);
            break;
        }
        default:
            break;
    }
}
