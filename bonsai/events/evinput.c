/**
 * @file bsi-input.c
 * @brief Holds all event handlers for `bsi_input_{pointer,keyboard}`.
 *
 *
 */

#include <assert.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>

#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/input/keyboard.h"
#include "bonsai/log.h"
#include "bonsai/server.h"

#define GIMME_ALL_POINTER_EVENTS
#define GIMME_ALL_KEYBOARD_EVENTS

void
handle_pointer_motion(struct wl_listener* listener, void* data)
{
    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.motion);
    struct bsi_server* server = pointer->server;
    struct wlr_pointer_motion_event* event = data;
    union bsi_cursor_event cursor_event = { .motion = event };

    /* Firstly, move the cursor. */
    wlr_cursor_move(
        pointer->cursor, pointer->device, event->delta_x, event->delta_y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(server, cursor_event);
}

void
handle_pointer_motion_absolute(struct wl_listener* listener, void* data)
{
    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.motion_absolute);
    struct bsi_server* server = pointer->server;
    struct wlr_pointer_motion_absolute_event* event = data;
    union bsi_cursor_event cursor_event = { .motion_absolute = event };

    /* Firstly, warp the absolute motion to our output layout constraints. */
    wlr_cursor_warp_absolute(
        server->wlr_cursor, pointer->device, event->x, event->y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(server, cursor_event);
}

void
handle_pointer_button(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event button from wlr_input_device");

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

    switch (event->state) {
        case WLR_BUTTON_RELEASED:
            /* Exit interactive mode. */
            server->cursor.cursor_mode = BSI_CURSOR_NORMAL;
            bsi_cursor_image_set(server, BSI_CURSOR_IMAGE_NORMAL);
            break;
        case WLR_BUTTON_PRESSED: {
            /* Get the view under pointer, its surface, and surface relative
             * pointer coordinates. */
            double sx, sy;
            struct wlr_scene_surface* scene_surface_at = NULL;
            struct wlr_surface* surface_at = NULL;
            const char* surface_role = NULL;
            void* scene_data = bsi_cursor_scene_data_at(server,
                                                        &scene_surface_at,
                                                        &surface_at,
                                                        &surface_role,
                                                        &sx,
                                                        &sy);

            if (scene_data == NULL) {
                wlr_seat_pointer_notify_clear_focus(wlr_seat);
                wlr_seat_keyboard_notify_clear_focus(wlr_seat);
                return;
            }

            /* Focus a client that was clicked. */
            if (strcmp(surface_role, "zwlr_layer_surface_v1") == 0) {
                bsi_layer_surface_focus(scene_data);
            } else if (strcmp(surface_role, "xdg_toplevel") == 0) {
                bsi_view_focus(scene_data);
            }
            break;
        }
    }
}

void
handle_pointer_axis(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event axis from wlr_input_device");

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
handle_pointer_frame(struct wl_listener* listener, void* data)
{
    struct bsi_input_pointer* pointer =
        wl_container_of(listener, pointer, listen.frame);
    struct wlr_seat* wlr_seat = pointer->server->wlr_seat;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_frame(wlr_seat);
}

void
handle_pointer_swipe_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_begin from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.swipe_begin);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_swipe_begin_event* event = data;

    // TODO: Server is primary handler.
}

void
handle_pointer_swipe_update(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_update from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.swipe_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_swipe_update_event* event = data;

    // TODO: Server is primary handler.
}

void
handle_pointer_swipe_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_end from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.swipe_end);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_swipe_end_event* event = data;

    // TODO: Server is primary handler.
}

void
handle_pointer_pinch_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_begin from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_begin);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_pinch_begin_event* event = data;
}

void
handle_pointer_pinch_update(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_update from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_pinch_update_event* event = data;
}

void
handle_pointer_pinch_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_end from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_pinch_end_event* event = data;
}

void
handle_pointer_hold_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event hold_begin from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_hold_begin_event* event = data;
}

void
handle_pointer_hold_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event hold_end from wlr_input_device");

    // struct bsi_input_pointer* pointer =
    //     wl_container_of(listener, pointer, listen.pinch_update);
    // struct wlr_seat* wlr_seat = pointer->bsi_server->wlr_seat;
    // struct wlr_pointer_hold_end_event* event = data;
}

void
handle_keyboard_key(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event key from wlr_input_device");

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
handle_keyboard_modifiers(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event modifiers from wlr_input_device");

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
handle_input_device_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_input_device");

    struct wlr_input_device* device = data;
    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD: {
            struct bsi_input_keyboard* keyboard =
                wl_container_of(listener, keyboard, listen.destroy);
            struct bsi_server* server = keyboard->server;

            bsi_debug("Destroying input device %s", keyboard->device->name);

            bsi_inputs_keyboard_remove(server, keyboard);
            bsi_input_keyboard_destroy(keyboard);
            break;
        }
        case WLR_INPUT_DEVICE_POINTER: {
            struct bsi_input_pointer* pointer =
                wl_container_of(listener, pointer, listen.destroy);
            struct bsi_server* server = pointer->server;

            bsi_debug("Destroying input device %s", pointer->device->name);

            bsi_inputs_pointer_remove(server, pointer);
            bsi_input_pointer_destroy(pointer);
            break;
        }
        default:
            break;
    }
}
