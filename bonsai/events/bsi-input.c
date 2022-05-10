/**
 * @file bsi-input.c
 * @brief Holds all event listeners for `bsi_input_{pointer,keyboard}`.
 *
 *
 */

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>

#include "bonsai/config/input.h"
#include "bonsai/cursor.h"
#include "bonsai/events.h"
#include "bonsai/scene/view.h"
#include "bonsai/server.h"

void
bsi_input_pointer_motion_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event motion from wlr_input_device");
#endif

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.motion);
    struct bsi_server* bsi_server = bsi_input_pointer->bsi_server;
    struct wlr_event_pointer_motion* event = data;
    union bsi_cursor_event bsi_cursor_event = { .motion = event };

    /* Firstly, move the cursor. */
    wlr_cursor_move(bsi_input_pointer->wlr_cursor,
                    bsi_input_pointer->wlr_input_device,
                    event->delta_x,
                    event->delta_y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(&bsi_server->bsi_cursor, bsi_cursor_event);
}

void
bsi_input_pointer_motion_absolute_notify(struct wl_listener* listener,
                                         void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event motion_absolute from wlr_input_device");
#endif

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.motion_absolute);
    struct bsi_server* bsi_server = bsi_input_pointer->bsi_server;
    struct wlr_event_pointer_motion_absolute* event = data;
    union bsi_cursor_event bsi_cursor_event = { .motion_absolute = event };

    /* Firstly, warp the absolute motion to our output layout constraints. */
    wlr_cursor_warp_absolute(bsi_server->wlr_cursor,
                             bsi_input_pointer->wlr_input_device,
                             event->x,
                             event->y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(&bsi_server->bsi_cursor, bsi_cursor_event);
}

void
bsi_input_pointer_button_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event button from wlr_input_device");
#endif

    /* This event is forwarded by wlr_cursor, when a pointer emits a button
     * event. */

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.button);
    struct bsi_cursor* bsi_cursor = &bsi_input_pointer->bsi_server->bsi_cursor;
    struct wlr_seat* wlr_seat = bsi_input_pointer->bsi_server->wlr_seat;
    struct wlr_event_pointer_button* event = data;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_button(
        wlr_seat, event->time_msec, event->button, event->state);

    /* Get the view under pointer, its surface, and surface relative pointer
     * coordinates. */
    double sx, sy;
    struct wlr_surface* surface = NULL;
    struct bsi_view* bsi_view =
        bsi_cursor_view_at(bsi_cursor, &surface, &sx, &sy);

    switch (event->state) {
        case WLR_BUTTON_RELEASED:
            /* Exit interactive mode. */
            bsi_cursor->cursor_mode = BSI_CURSOR_NORMAL;
            break;
        case WLR_BUTTON_PRESSED:
            /* Focus a client that was clicked. */
            if (bsi_view != NULL)
                bsi_view_focus(bsi_view);
            break;
    }
}

void
bsi_input_pointer_axis_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event axis from wlr_input_device");
#endif

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.axis);
    struct wlr_seat* wlr_seat = bsi_input_pointer->bsi_server->wlr_seat;
    struct wlr_event_pointer_axis* event = data;

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

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.frame);
    struct wlr_seat* wlr_seat = bsi_input_pointer->bsi_server->wlr_seat;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_frame(wlr_seat);
}

void
bsi_input_pointer_swipe_begin_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event swipe_begin from wlr_input_device");
#endif

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.swipe_begin);
    struct wlr_seat* wlr_seat = bsi_input_pointer->bsi_server->wlr_seat;
    struct wlr_event_pointer_swipe_begin* event = data;

    // TODO: Server is primary handler.
#warning "Not implemented"
}

void
bsi_input_pointer_swipe_update_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event swipe_update from wlr_input_device");
#endif

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.swipe_update);
    struct wlr_seat* wlr_seat = bsi_input_pointer->bsi_server->wlr_seat;
    struct wlr_event_pointer_swipe_update* event = data;

    // TODO: Server is primary handler.
#warning "Not implemented"
}

void
bsi_input_pointer_swipe_end_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event swipe_end from wlr_input_device");
#endif

    struct bsi_input_pointer* bsi_input_pointer =
        wl_container_of(listener, bsi_input_pointer, events.swipe_end);
    struct wlr_seat* wlr_seat = bsi_input_pointer->bsi_server->wlr_seat;
    struct wlr_event_pointer_swipe_end* event = data;

    // TODO: Server is primary handler.
#warning "Not implemented"
}

void
bsi_input_pointer_pinch_begin_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event pinch_begin from wlr_input_device");
#endif
#warning "Not implemented"
}

void
bsi_input_pointer_pinch_update_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event pinch_update from wlr_input_device");
#endif
#warning "Not implemented"
}

void
bsi_input_pointer_pinch_end_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event pinch_end from wlr_input_device");
#endif
#warning "Not implemented"
}

void
bsi_input_pointer_hold_begin_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event hold_begin from wlr_input_device");
#endif
#warning "Not implemented"
}

void
bsi_input_pointer_hold_end_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_POINTER_EVENTS
    wlr_log(WLR_DEBUG, "Got event hold_end from wlr_input_device");
#endif
#warning "Not implemented"
}

void
bsi_input_keyboard_key_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event key from wlr_input_device");
#endif

    // TODO: Server is primary handler.

#warning "Not implemented"
}

void
bsi_input_keyboard_modifiers_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event modifiers from wlr_input_device");
#endif

    // TODO: Server is primary handler.

#warning "Not implemented"
}

void
bsi_input_keyboard_keymap_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event keymap from wlr_input_device");
#endif

    // TODO: Server is only handler.

#warning "Not implemented"
}

void
bsi_input_keyboard_repeat_info_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event repeat_info from wlr_input_device");
#endif
#warning "Not implemented"
}

// TODO: A pointer destroy listener? Idk why a keyboard has one, pointer not.

void
bsi_input_keyboard_destroy_notify(struct wl_listener* listener, void* data)
{
#ifdef GIMME_ALL_KEYBOARD_EVENTS
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_input_device");
#endif

    struct bsi_input_keyboard* bsi_input_keyboard =
        wl_container_of(listener, bsi_input_keyboard, events.destroy);
    struct bsi_inputs* bsi_inputs = &bsi_input_keyboard->bsi_server->bsi_inputs;

    bsi_inputs_keyboard_remove(bsi_inputs, bsi_input_keyboard);
    bsi_input_keyboard_destroy(bsi_input_keyboard);
}
