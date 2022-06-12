#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/backend/libinput.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
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
#include "bonsai/util.h"

struct bsi_input_device*
input_device_init(struct bsi_input_device* input_device,
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
input_device_destroy(struct bsi_input_device* input_device)
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
input_device_keymap_set(struct bsi_input_device* input_device,
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

/* Handlers */
static void
handle_motion(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.motion);
    struct bsi_server* server = device->server;
    struct wlr_pointer_motion_event* event = data;
    union bsi_cursor_event cursor_event = { .motion = event };

    /* Firstly, move the cursor and notify seat of activity. */
    wlr_cursor_move(
        device->cursor, device->device, event->delta_x, event->delta_y);

    wlr_idle_notify_activity(server->wlr_idle, server->wlr_seat);

    if (server->session.locked)
        return;

    /* Secondly, check if we have any view stuff to do. */
    cursor_process_motion(server, cursor_event);
}

static void
handle_motion_absolute(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.motion_absolute);
    struct bsi_server* server = device->server;
    struct wlr_pointer_motion_absolute_event* event = data;
    union bsi_cursor_event cursor_event = { .motion_absolute = event };

    /* Firstly, warp the absolute motion to our output layout constraints. */
    wlr_cursor_warp_absolute(
        server->wlr_cursor, device->device, event->x, event->y);

    if (server->session.locked)
        return;

    /* Secondly, check if we have any view stuff to do. */
    cursor_process_motion(server, cursor_event);
}

static void
handle_button(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event button from wlr_input_device");

    /* This event is forwarded by wlr_cursor, when a pointer emits a button
     * event. */

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.button);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* seat = device->server->wlr_seat;
    struct wlr_pointer_button_event* event = data;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_button(
        seat, event->time_msec, event->button, event->state);

    wlr_idle_notify_activity(server->wlr_idle, server->wlr_seat);

    switch (event->state) {
        case WLR_BUTTON_RELEASED:
            /* Exit interactive mode. */
            server->cursor.cursor_mode = BSI_CURSOR_NORMAL;
            cursor_image_set(server, BSI_CURSOR_IMAGE_NORMAL);
            break;
        case WLR_BUTTON_PRESSED: {
            /* Get the view under pointer, its surface, and surface relative
             * pointer coordinates. */
            double sx, sy;
            struct wlr_scene_surface* scene_surface_at = NULL;
            struct wlr_surface* surface_at = NULL;
            const char* surface_role = NULL;
            void* scene_data = cursor_scene_data_at(server,
                                                    &scene_surface_at,
                                                    &surface_at,
                                                    &surface_role,
                                                    &sx,
                                                    &sy);

            if (scene_data == NULL) {
                wlr_seat_pointer_notify_clear_focus(seat);
                wlr_seat_keyboard_notify_clear_focus(seat);
                return;
            }

            /* Focus a client that was clicked. */
            if (wlr_surface_is_layer_surface(surface_at)) {
                layer_surface_focus(scene_data);
            } else if (wlr_surface_is_xdg_surface(surface_at)) {
                view_focus(scene_data);
            } else if (wlr_surface_is_subsurface(surface_at)) {
                /* Client side decorations are wl_subsurfaces. */
                struct bsi_view* view = scene_data;
                struct wlr_cursor* cursor = view->server->wlr_cursor;
                double sub_sx, sub_sy;
                if (wlr_xdg_surface_surface_at(view->wlr_xdg_toplevel->base,
                                               cursor->x - view->geom.x,
                                               cursor->y - view->geom.y,
                                               &sub_sx,
                                               &sub_sy))
                    view_focus(view);
            }
            break;
        }
    }
}

static void
handle_axis(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.axis);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* wlr_seat = server->wlr_seat;
    struct wlr_pointer_axis_event* event = data;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_axis(wlr_seat,
                                 event->time_msec,
                                 event->orientation,
                                 event->delta,
                                 event->delta_discrete,
                                 event->source);

    wlr_idle_notify_activity(server->wlr_idle, server->wlr_seat);
}

static void
handle_frame(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.frame);
    struct bsi_server* server = device->server;
    struct wlr_seat* seat = server->wlr_seat;

    /* Notify client that has pointer focus of the event. */
    wlr_seat_pointer_notify_frame(seat);
}

static void
handle_swipe_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_begin from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.swipe_begin);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_pointer_swipe_begin_event* event = data;

    bsi_debug("swipe_begin { time_msec=%u, fingers=%u }",
              event->time_msec,
              event->fingers);

    server->cursor.cursor_mode = BSI_CURSOR_SWIPE;
    server->cursor.swipe_fingers = event->fingers;
    server->cursor.swipe_timest = event->time_msec;

    wlr_idle_notify_activity(server->wlr_idle, server->wlr_seat);
}

static void
handle_swipe_update(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.swipe_update);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_pointer_swipe_update_event* event = data;

    bsi_debug("swipe_update { time_msec=%u, fingers=%u, dx=%.3f, dy=%.3f }",
              event->time_msec,
              event->fingers,
              event->dx,
              event->dy);

    union bsi_cursor_event cursor_event = { .swipe_update = event };
    cursor_process_swipe(server, cursor_event);
}

static void
handle_swipe_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event swipe_end from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.swipe_end);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_pointer_swipe_end_event* event = data;

    bsi_debug("swipe_end { time_msec=%u, cancelled=%d }",
              event->time_msec,
              event->cancelled);

    server->cursor.cursor_mode = BSI_CURSOR_NORMAL;
    server->cursor.swipe_cancelled = event->cancelled;
    server->cursor.swipe_timest = event->time_msec;

    if (event->cancelled) {
        server->cursor.swipe_dx = 0.0;
        server->cursor.swipe_dy = 0.0;
        server->cursor.swipe_fingers = 0;
        server->cursor.swipe_timest = 0;
        server->cursor.swipe_cancelled = false;
        return;
    }

    struct wlr_output* wlr_output =
        wlr_output_layout_output_at(server->wlr_output_layout,
                                    server->wlr_cursor->x,
                                    server->wlr_cursor->y);
    struct bsi_output* output = outputs_find(server, wlr_output);
    // struct bsi_workspace* active_workspace =
    // bsi_workspaces_get_active(output);

    bsi_debug("Accumulated swipe is { swipe_dx=%.3f, swipe_dy=%.3f }",
              server->cursor.swipe_dx,
              server->cursor.swipe_dy);

    if (fabs(server->cursor.swipe_dx) > fabs(server->cursor.swipe_dy)) {
        /* Switch workspaces. We use inverted directions for natrual swiping. */
        if (server->cursor.swipe_dx > 0)
            workspaces_prev(output);
        else
            workspaces_next(output);
    } else {
        /* Show/hide all views. */
        if (server->cursor.swipe_dy > 0)
            workspace_views_show_all(server->active_workspace, false);
        else
            workspace_views_show_all(server->active_workspace, true);
    }

    server->cursor.swipe_dx = 0.0;
    server->cursor.swipe_dy = 0.0;
    server->cursor.swipe_fingers = 0;
    server->cursor.swipe_timest = 0;
    server->cursor.swipe_cancelled = false;
}

static void
handle_pinch_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_begin from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_begin);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_pointer_pinch_begin_event* event = data;

    wlr_idle_notify_activity(device->server->wlr_idle, seat);
}

static void
handle_pinch_update(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_update);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_pointer_pinch_update_event* event = data;
}

static void
handle_pinch_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pinch_end from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.pinch_end);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_pointer_pinch_end_event* event = data;
}

static void
handle_hold_begin(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event hold_begin from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.hold_begin);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_pointer_hold_begin_event* event = data;

    wlr_idle_notify_activity(device->server->wlr_idle, seat);
}

static void
handle_hold_end(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event hold_end from wlr_input_device");

    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.hold_end);
    struct bsi_server* server = device->server;

    if (server->session.locked)
        return;

    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_pointer_hold_end_event* event = data;
}

static void
handle_key(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.key);
    struct bsi_server* server = device->server;
    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_keyboard_key_event* event = data;

    wlr_idle_notify_activity(device->server->wlr_idle, seat);

    if (!keyboard_keybinds_process(device, event)) {
        /* A seat can only have one keyboard, but multiple keyboards can be
         * attached. Switch the seat to the correct underlying keyboard. Roots
         * handles this for us :). */
        wlr_seat_set_keyboard(seat, device->device->keyboard);
        /* The server knows not thy keybind, the client shall handle it. Notify
         * client of keys not handled by the server. */
        wlr_seat_keyboard_notify_key(
            seat, event->time_msec, event->keycode, event->state);
    }
}

static void
handle_modifiers(struct wl_listener* listener, void* data)
{
    struct bsi_input_device* device =
        wl_container_of(listener, device, listen.modifiers);
    struct bsi_server* server = device->server;
    struct wlr_seat* seat = server->wlr_seat;
    struct wlr_input_device* dev = device->device;

    wlr_idle_notify_activity(device->server->wlr_idle, seat);

    /* A seat can only have one keyboard, but multiple keyboards can be
     * attached. Switch the seat to the correct underlying keyboard. Roots
     * handles this for us :). */
    wlr_seat_set_keyboard(seat, dev->keyboard);
    /* Notify client of keys not handled by the server. */
    wlr_seat_keyboard_notify_modifiers(seat, &dev->keyboard->modifiers);
}

static void
handle_destroy(struct wl_listener* listener, void* data)
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

    inputs_remove(device);
    input_device_destroy(device);
}

void
handle_new_input(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_input from wlr_backend");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_input);
    struct wlr_input_device* wlr_device = data;

    switch (wlr_device->type) {
        case WLR_INPUT_DEVICE_POINTER: {
            struct bsi_input_device* device =
                calloc(1, sizeof(struct bsi_input_device));
            input_device_init(
                device, BSI_INPUT_DEVICE_POINTER, server, wlr_device);
            inputs_add(server, device);

            util_slot_connect(&device->cursor->events.motion,
                              &device->listen.motion,
                              handle_motion);
            util_slot_connect(&device->cursor->events.motion_absolute,
                              &device->listen.motion_absolute,
                              handle_motion_absolute);
            util_slot_connect(&device->cursor->events.button,
                              &device->listen.button,
                              handle_button);
            util_slot_connect(&device->cursor->events.axis,
                              &device->listen.axis,
                              handle_axis);
            util_slot_connect(&device->cursor->events.frame,
                              &device->listen.frame,
                              handle_frame);
            util_slot_connect(&device->cursor->events.swipe_begin,
                              &device->listen.swipe_begin,
                              handle_swipe_begin);
            util_slot_connect(&device->cursor->events.swipe_update,
                              &device->listen.swipe_update,
                              handle_swipe_update);
            util_slot_connect(&device->cursor->events.swipe_end,
                              &device->listen.swipe_end,
                              handle_swipe_end);
            util_slot_connect(&device->cursor->events.pinch_begin,
                              &device->listen.pinch_begin,
                              handle_pinch_begin);
            util_slot_connect(&device->cursor->events.pinch_update,
                              &device->listen.pinch_update,
                              handle_pinch_update);
            util_slot_connect(&device->cursor->events.pinch_end,
                              &device->listen.pinch_end,
                              handle_pinch_end);
            util_slot_connect(&device->cursor->events.hold_begin,
                              &device->listen.hold_begin,
                              handle_hold_begin);
            util_slot_connect(&device->cursor->events.hold_end,
                              &device->listen.hold_end,
                              handle_hold_end);
            util_slot_connect(&device->device->events.destroy,
                              &device->listen.destroy,
                              handle_destroy);

            wlr_cursor_attach_input_device(device->cursor, device->device);

            if (wlr_input_device_is_libinput(device->device)) {
                struct libinput_device* libinput_dev =
                    wlr_libinput_get_device_handle(device->device);

                /* Enable tap to click. */
                libinput_device_config_tap_set_enabled(
                    libinput_dev, LIBINPUT_CONFIG_TAP_ENABLED);

                bool has_config = false;
                struct bsi_config_input* conf;
                wl_list_for_each(conf, &server->config.input, link)
                {
                    if (strcasecmp(conf->device_name, device->device->name) ==
                        0) {
                        bsi_debug("Matched config for input device '%s'",
                                  device->device->name);
                        has_config = true;
                        switch (conf->type) {
                            case BSI_CONFIG_INPUT_POINTER_ACCEL_SPEED: {
                                double speed =
                                    (conf->accel_speed > 0.0)
                                        ? fmax(conf->accel_speed, 1.0)
                                        : fmax(conf->accel_speed, -1.0);
                                libinput_device_config_accel_set_speed(
                                    libinput_dev, speed);
                                bsi_debug("Set accel_speed %.2f", speed);
                                break;
                            }
                            case BSI_CONFIG_INPUT_POINTER_ACCEL_PROFILE:
                                libinput_device_config_accel_set_profile(
                                    libinput_dev, conf->accel_profile);
                                bsi_debug("Set accel_profile %d",
                                          conf->accel_profile);
                                break;
                            case BSI_CONFIG_INPUT_POINTER_SCROLL_NATURAL:
                                libinput_device_config_scroll_set_natural_scroll_enabled(
                                    libinput_dev, conf->natural_scroll);
                                bsi_debug("Set natural scroll %d",
                                          conf->natural_scroll);
                                break;
                            default:
                                break;
                        }
                    }
                }

                if (!has_config)
                    bsi_info("No matching config for input device '%s'",
                             device->device->name);
            } else {
                bsi_info("Device '%s' doesn't use libinput backend, skipping",
                         device->device->name);
            }

            bsi_info(
                "Added new pointer input device '%s' (vendor %x, product %x)",
                device->device->name,
                device->device->vendor,
                device->device->product);
            break;
        }
        case WLR_INPUT_DEVICE_KEYBOARD: {
            struct bsi_input_device* device =
                calloc(1, sizeof(struct bsi_input_device));
            input_device_init(
                device, BSI_INPUT_DEVICE_KEYBOARD, server, wlr_device);
            inputs_add(server, device);

            util_slot_connect(&device->device->keyboard->events.key,
                              &device->listen.key,
                              handle_key);
            util_slot_connect(&device->device->keyboard->events.modifiers,
                              &device->listen.modifiers,
                              handle_modifiers);
            util_slot_connect(&device->device->events.destroy,
                              &device->listen.destroy,
                              handle_destroy);

            bool has_config = false;
            struct bsi_config_input* conf;
            wl_list_for_each(conf, &server->config.input, link)
            {
                if (strcasecmp(conf->device_name, device->device->name) == 0) {
                    bsi_debug("Matched config for input device '%s'",
                              device->device->name);
                    has_config = true;
                    switch (conf->type) {
                        case BSI_CONFIG_INPUT_KEYBOARD_LAYOUT:
                            break;
                        case BSI_CONFIG_INPUT_KEYBOARD_REPEAT_INFO:
                            wlr_keyboard_set_repeat_info(
                                device->device->keyboard,
                                conf->repeat_rate,
                                conf->delay);
                            bsi_debug("Set repeat info { rate=%d, delay=%d }",
                                      conf->repeat_rate,
                                      conf->delay);
                            break;
                        default:
                            break;
                    }
                }
            }

            if (!has_config)
                bsi_info("No matching config for input device '%s'",
                         device->device->name);

            input_device_keymap_set(
                device, bsi_input_keyboard_rules, bsi_input_keyboard_rules_len);

            wlr_seat_set_keyboard(server->wlr_seat, device->device->keyboard);

            bsi_info(
                "Added new keyboard input device '%s' (vendor %x, product %x)",
                device->device->name,
                device->device->vendor,
                device->device->product);
            break;
        }
        default:
            bsi_info("Unsupported new input device '%s' (type %x, vendor %x, "
                     "product %x)",
                     wlr_device->name,
                     wlr_device->type,
                     wlr_device->vendor,
                     wlr_device->product);
            break;
    }

    uint32_t capabilities = 0;
    size_t len_keyboards = 0, len_pointers = 0;
    struct bsi_input_device* device;
    wl_list_for_each(device, &server->input.inputs, link_server)
    {
        switch (device->type) {
            case BSI_INPUT_DEVICE_POINTER:
                ++len_pointers;
                capabilities |= WL_SEAT_CAPABILITY_POINTER;
                bsi_debug("Seat has capability: WL_SEAT_CAPABILITY_POINTER");
                break;
            case BSI_INPUT_DEVICE_KEYBOARD:
                ++len_keyboards;
                capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
                bsi_debug("Seat has capability: WL_SEAT_CAPABILITY_KEYBOARD");
                break;
        }
    }

    bsi_debug("Server now has %ld input pointer devices", len_pointers);
    bsi_debug("Server now has %ld input keyboard devices", len_keyboards);

    wlr_seat_set_capabilities(server->wlr_seat, capabilities);
}
