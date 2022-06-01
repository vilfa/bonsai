#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

#include "bonsai/desktop/layers.h"
#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/input/keyboard.h"
#include "bonsai/log.h"
#include "bonsai/server.h"

void
bsi_inputs_add(struct bsi_server* server, struct bsi_input_device* device)
{
    wl_list_insert(&server->input.inputs, &device->link_server);
}

void
bsi_inputs_remove(struct bsi_input_device* device)
{
    wl_list_remove(&device->link_server);
}

struct bsi_input_device*
bsi_input_device_init(struct bsi_input_device* input_device,
                      enum bsi_input_device_type type,
                      struct bsi_server* server,
                      struct wlr_input_device* device)
{
    switch (type) {
        case BSI_INPUT_DEVICE_POINTER:
            input_device->server = server;
            input_device->cursor = server->wlr_cursor;
            input_device->device = device;
            input_device->type = type;
            break;
        case BSI_INPUT_DEVICE_KEYBOARD:
            input_device->server = server;
            input_device->device = device;
            input_device->type = type;
            break;
    }

    return input_device;
}

void
bsi_input_device_destroy(struct bsi_input_device* input_device)
{
    switch (input_device->type) {
        case BSI_INPUT_DEVICE_POINTER:
            wl_list_remove(&input_device->listen.motion.link);
            wl_list_remove(&input_device->listen.motion_absolute.link);
            wl_list_remove(&input_device->listen.button.link);
            wl_list_remove(&input_device->listen.axis.link);
            wl_list_remove(&input_device->listen.frame.link);
            wl_list_remove(&input_device->listen.swipe_begin.link);
            wl_list_remove(&input_device->listen.swipe_update.link);
            wl_list_remove(&input_device->listen.swipe_end.link);
            wl_list_remove(&input_device->listen.pinch_begin.link);
            wl_list_remove(&input_device->listen.pinch_update.link);
            wl_list_remove(&input_device->listen.pinch_end.link);
            wl_list_remove(&input_device->listen.hold_begin.link);
            wl_list_remove(&input_device->listen.hold_end.link);
            break;
        case BSI_INPUT_DEVICE_KEYBOARD:
            wl_list_remove(&input_device->listen.key.link);
            wl_list_remove(&input_device->listen.modifiers.link);
            wl_list_remove(&input_device->listen.destroy.link);
            break;
    }
    free(input_device);
}

void
bsi_input_device_keymap_set(struct bsi_input_device* input_device,
                            const struct xkb_rule_names* xkb_rule_names,
                            const size_t xkb_rule_names_len)
{
    assert(input_device->type == BSI_INPUT_DEVICE_KEYBOARD);

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

    wlr_keyboard_set_keymap(input_device->device->keyboard, xkb_keymap);

    xkb_keymap_unref(xkb_keymap);
    xkb_context_unref(xkb_context);
}

/**
 * Handlers
 */
void
handle_pointer_motion(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.motion);
    struct bsi_server* server = device->server;
    struct wlr_pointer_motion_event* event = data;
    union bsi_cursor_event cursor_event = { .motion = event };

    /* Firstly, move the cursor. */
    wlr_cursor_move(
        device->cursor, device->device, event->delta_x, event->delta_y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(server, cursor_event);
}

void
handle_pointer_motion_absolute(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.motion_absolute);
    struct bsi_server* server = device->server;
    struct wlr_pointer_motion_absolute_event* event = data;
    union bsi_cursor_event cursor_event = { .motion_absolute = event };

    /* Firstly, warp the absolute motion to our output layout constraints. */
    wlr_cursor_warp_absolute(
        server->wlr_cursor, device->device, event->x, event->y);

    /* Secondly, check if we have any view stuff to do. */
    bsi_cursor_process_motion(server, cursor_event);
}

void
handle_pointer_button(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event button from wlr_input_device");

    /* This event is forwarded by wlr_cursor, when a pointer emits a button
     * event. */

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.button);
    struct bsi_server* server = device->server;
    struct wlr_seat* wlr_seat = device->server->wlr_seat;
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
            } else if (strcmp(surface_role, "wl_subsurface") == 0) {
                /* Client side decorations are wl_subsurfaces. */
                struct bsi_view* view = scene_data;
                struct wlr_cursor* cursor = view->server->wlr_cursor;
                double sub_sx, sub_sy;
                if (wlr_xdg_surface_surface_at(view->toplevel->base,
                                               cursor->x - view->box.x,
                                               cursor->y - view->box.y,
                                               &sub_sx,
                                               &sub_sy))
                    bsi_view_focus(view);
            }
            break;
        }
    }
}

void
handle_pointer_axis(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event axis from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.axis);
    struct wlr_seat* wlr_seat = device->server->wlr_seat;
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
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.frame);
    struct wlr_seat* wlr_seat = device->server->wlr_seat;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_frame(wlr_seat);
}

void
handle_pointer_swipe_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_begin from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.swipe_begin);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_swipe_begin_event* event = data;

    // TODO: Server is primary handler.
}

void
handle_pointer_swipe_update(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_update from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.swipe_update);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_swipe_update_event* event = data;

    // TODO: Server is primary handler.
}

void
handle_pointer_swipe_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_end from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.swipe_end);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_swipe_end_event* event = data;

    // TODO: Server is primary handler.
}

void
handle_pointer_pinch_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_begin from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_begin);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_pinch_begin_event* event = data;
}

void
handle_pointer_pinch_update(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_update from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_update);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_pinch_update_event* event = data;
}

void
handle_pointer_pinch_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_end from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_update);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_pinch_end_event* event = data;
}

void
handle_pointer_hold_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event hold_begin from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_update);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_hold_begin_event* event = data;
}

void
handle_pointer_hold_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event hold_end from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_update);
    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_hold_end_event* event = data;
}

void
handle_keyboard_key(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event key from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.key);
    struct wlr_seat* wlr_seat = device->server->wlr_seat;
    struct wlr_keyboard_key_event* event = data;

    if (!bsi_keyboard_keybinds_process(device, event)) {
        /* A seat can only have one keyboard, but multiple keyboards can be
         * attached. Switch the seat to the correct underlying keyboard. Roots
         * handles this for us :). */
        wlr_seat_set_keyboard(wlr_seat, device->device->keyboard);
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

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.modifiers);
    struct wlr_seat* wlr_seat = device->server->wlr_seat;
    struct wlr_input_device* dev = device->device;

    /* A seat can only have one keyboard, but multiple keyboards can be
     * attached. Switch the seat to the correct underlying keyboard. Roots
     * handles this for us :). */
    wlr_seat_set_keyboard(wlr_seat, dev->keyboard);
    /* Notify client of keys not handled by the server. */
    wlr_seat_keyboard_notify_modifiers(wlr_seat, &dev->keyboard->modifiers);
}

void
handle_input_device_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.destroy);
    struct wlr_input_device* wlr_device = data;
    switch (device->type) {
        case BSI_INPUT_DEVICE_POINTER:
            bsi_debug("Destroying input pointer '%s'", device->device->name);
            break;
        case BSI_INPUT_DEVICE_KEYBOARD:
            bsi_debug("Destroying input keyboard '%s'", device->device->name);
            break;
        default:
            bsi_debug("Unknown input device '%s'", wlr_device->name);
            break;
    }

    bsi_inputs_remove(device);
    bsi_input_device_destroy(device);
}
