/**
 * @file bsi-listeners.c
 * @brief Contains all event listeners for `bsi_listeners`.
 *
 *
 */

#include <wayland-server-core.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/config/input.h"
#include "bonsai/events.h"
#include "bonsai/server.h"

void
bsi_listeners_backend_new_output_notify(struct wl_listener* listener,
                                        void* data)
{
    wlr_log(WLR_DEBUG, "Got event new_output from wlr_backend");

    struct wlr_output* wlr_output = data;
    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_backend.new_output);

    wlr_output_init_render(
        wlr_output, bsi_server->wlr_allocator, bsi_server->wlr_renderer);

    // TODO: Allow user to pick output mode.

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode =
            wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output))
            return;
    }

    struct bsi_output* bsi_output = calloc(1, sizeof(struct bsi_output));
    bsi_output_init(bsi_output, bsi_server, wlr_output);
    bsi_outputs_add(&bsi_server->bsi_outputs, bsi_output);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_FRAME,
                            &bsi_output->events.frame,
                            &bsi_output->wlr_output->events.frame,
                            bsi_output_frame_notify);

    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_DAMAGE,
                            &bsi_output->events.damage,
                            &bsi_output->wlr_output->events.damage,
                            bsi_output_damage_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_NEEDS_FRAME,
                            &bsi_output->events.needs_frame,
                            &bsi_output->wlr_output->events.needs_frame,
                            bsi_output_needs_frame_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_PRECOMMIT,
                            &bsi_output->events.precommit,
                            &bsi_output->wlr_output->events.precommit,
                            bsi_output_precommit_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_COMMIT,
                            &bsi_output->events.commit,
                            &bsi_output->wlr_output->events.commit,
                            bsi_output_commit_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_PRESENT,
                            &bsi_output->events.present,
                            &bsi_output->wlr_output->events.present,
                            bsi_output_present_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_BIND,
                            &bsi_output->events.bind,
                            &bsi_output->wlr_output->events.bind,
                            bsi_output_bind_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_ENABLE,
                            &bsi_output->events.enable,
                            &bsi_output->wlr_output->events.enable,
                            bsi_output_enable_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_MODE,
                            &bsi_output->events.mode,
                            &bsi_output->wlr_output->events.mode,
                            bsi_output_mode_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_DESCRIPTION,
                            &bsi_output->events.description,
                            &bsi_output->wlr_output->events.description,
                            bsi_output_description_notify);
    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_DESTROY,
                            &bsi_output->events.destroy,
                            &bsi_output->wlr_output->events.destroy,
                            bsi_output_destroy_notify);

    wlr_output_layout_add_auto(bsi_server->wlr_output_layout, wlr_output);
}

void
bsi_listeners_backend_new_input_notify(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "Got event new_input from wlr_backend");

    struct wlr_input_device* wlr_input_device = data;
    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_backend.new_input);

    switch (wlr_input_device->type) {
        case WLR_INPUT_DEVICE_POINTER: {
            struct bsi_input_pointer* bsi_input_pointer =
                calloc(1, sizeof(struct bsi_input_pointer));
            bsi_input_pointer_init(
                bsi_input_pointer, bsi_server, wlr_input_device);
            bsi_inputs_add_pointer(&bsi_server->bsi_inputs, bsi_input_pointer);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_MOTION,
                &bsi_input_pointer->events.motion,
                &bsi_input_pointer->wlr_cursor->events.motion,
                bsi_input_pointer_motion_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_MOTION_ABSOLUTE,
                &bsi_input_pointer->events.motion_absolute,
                &bsi_input_pointer->wlr_cursor->events.motion_absolute,
                bsi_input_pointer_motion_absolute_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_BUTTON,
                &bsi_input_pointer->events.button,
                &bsi_input_pointer->wlr_cursor->events.button,
                bsi_input_pointer_button_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_AXIS,
                &bsi_input_pointer->events.axis,
                &bsi_input_pointer->wlr_cursor->events.axis,
                bsi_input_pointer_axis_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_FRAME,
                &bsi_input_pointer->events.frame,
                &bsi_input_pointer->wlr_cursor->events.frame,
                bsi_input_pointer_frame_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_SWIPE_BEGIN,
                &bsi_input_pointer->events.swipe_begin,
                &bsi_input_pointer->wlr_cursor->events.swipe_begin,
                bsi_input_pointer_swipe_begin_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_SWIPE_UPDATE,
                &bsi_input_pointer->events.swipe_update,
                &bsi_input_pointer->wlr_cursor->events.swipe_update,
                bsi_input_pointer_swipe_update_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_SWIPE_END,
                &bsi_input_pointer->events.swipe_end,
                &bsi_input_pointer->wlr_cursor->events.swipe_end,
                bsi_input_pointer_swipe_end_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_PINCH_BEGIN,
                &bsi_input_pointer->events.pinch_begin,
                &bsi_input_pointer->wlr_cursor->events.pinch_begin,
                bsi_input_pointer_pinch_begin_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_PINCH_UPDATE,
                &bsi_input_pointer->events.pinch_update,
                &bsi_input_pointer->wlr_cursor->events.pinch_update,
                bsi_input_pointer_pinch_update_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_PINCH_END,
                &bsi_input_pointer->events.pinch_end,
                &bsi_input_pointer->wlr_cursor->events.pinch_end,
                bsi_input_pointer_pinch_end_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_HOLD_BEGIN,
                &bsi_input_pointer->events.hold_begin,
                &bsi_input_pointer->wlr_cursor->events.hold_begin,
                bsi_input_pointer_hold_begin_notify);
            bsi_input_pointer_add_listener(
                bsi_input_pointer,
                BSI_INPUT_POINTER_LISTENER_HOLD_END,
                &bsi_input_pointer->events.hold_end,
                &bsi_input_pointer->wlr_cursor->events.hold_end,
                bsi_input_pointer_hold_end_notify);
            wlr_log(WLR_DEBUG, "Added new pointer input device");
            break;
        }
        case WLR_INPUT_DEVICE_KEYBOARD: {
            struct bsi_input_keyboard* bsi_input_keyboard =
                calloc(1, sizeof(struct bsi_input_keyboard));
            bsi_input_keyboard_init(
                bsi_input_keyboard, bsi_server, wlr_input_device);
            bsi_inputs_add_keyboard(&bsi_server->bsi_inputs,
                                    bsi_input_keyboard);
            bsi_input_keyboard_add_listener(
                bsi_input_keyboard,
                BSI_INPUT_KEYBOARD_LISTENER_KEY,
                &bsi_input_keyboard->events.key,
                &bsi_input_keyboard->wlr_input_device->keyboard->events.key,
                bsi_input_keyboard_key_notify);
            bsi_input_keyboard_add_listener(
                bsi_input_keyboard,
                BSI_INPUT_KEYBOARD_LISTENER_MODIFIERS,
                &bsi_input_keyboard->events.modifiers,
                &bsi_input_keyboard->wlr_input_device->keyboard->events
                     .modifiers,
                bsi_input_keyboard_modifiers_notify);
            bsi_input_keyboard_add_listener(
                bsi_input_keyboard,
                BSI_INPUT_KEYBOARD_LISTENER_KEYMAP,
                &bsi_input_keyboard->events.keymap,
                &bsi_input_keyboard->wlr_input_device->keyboard->events.keymap,
                bsi_input_keyboard_keymap_notify);
            bsi_input_keyboard_add_listener(
                bsi_input_keyboard,
                BSI_INPUT_KEYBOARD_LISTENER_REPEAT_INFO,
                &bsi_input_keyboard->events.repeat_info,
                &bsi_input_keyboard->wlr_input_device->keyboard->events
                     .repeat_info,
                bsi_input_keyboard_repeat_info_notify);
            bsi_input_keyboard_add_listener(
                bsi_input_keyboard,
                BSI_INPUT_KEYBOARD_LISTENER_DESTROY,
                &bsi_input_keyboard->events.destroy,
                &bsi_input_keyboard->wlr_input_device->keyboard->events.destroy,
                bsi_input_keyboard_destroy_notify);
            wlr_log(WLR_DEBUG, "Added new keyboard input device");
            break;
        }
        default:
            wlr_log(WLR_INFO,
                    "Unsupported new input device: type %d",
                    wlr_input_device->type);
            break;
    }

    uint32_t capabilities = 0;
    size_t len_keyboards = 0, len_pointers = 0;
    if ((len_pointers = bsi_inputs_len_pointers(&bsi_server->bsi_inputs)) > 0) {
        capabilities |= WL_SEAT_CAPABILITY_POINTER;
        wlr_log(WLR_DEBUG, "Seat has capability: WL_SEAT_CAPABILITY_POINTER");
    }
    if ((len_keyboards = bsi_inputs_len_keyboard(&bsi_server->bsi_inputs)) >
        0) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
        wlr_log(WLR_DEBUG, "Seat has capability: WL_SEAT_CAPABILITY_KEYBOARD");
    }

    wlr_log(
        WLR_DEBUG, "Server now has %ld input pointer devices", len_pointers);
    wlr_log(
        WLR_DEBUG, "Server now has %ld input keyboard devices", len_keyboards);

    wlr_seat_set_capabilities(bsi_server->wlr_seat, capabilities);
}

void
bsi_listeners_backend_destroy_notify(struct wl_listener* listener,
                                     __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_backend, exiting");

    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_backend.destroy);

    bsi_server_exit(bsi_server);
}

void
bsi_listeners_seat_pointer_grab_begin_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event pointer_grab_begin from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_pointer_grab_end_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event pointer_grab_end from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_keyboard_grab_begin_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event keyboard_grab_begin from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_keyboard_grab_end_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event keyboard_grab_end from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_touch_grab_begin_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event touch_grab_begin from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_touch_grab_end_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event touch_grab_end from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_request_set_cursor_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event request_set_cursor from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_request_set_selection_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event request_set_selection from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_set_selection_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event set_selection from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_request_set_primary_selection_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event request_set_primary_selection from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_set_primary_selection_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event set_primary_selection from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_request_start_drag_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event request_start_drag from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_start_drag_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event start_drag from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_seat_destroy_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_seat");
#warning "Not implemented"
}

void
bsi_listeners_xdg_shell_new_surface_notify(struct wl_listener* listener,
                                           void* data)
{
    wlr_log(WLR_DEBUG, "Got event new_surface from wlr_xdg_shell");

    struct wlr_xdg_surface* wlr_xdg_surface = data;
    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_xdg_shell.new_surface);

    /* Firstly check if wlr_xdg_surface is a popup surface. If it is not a popup
     * surface, then it is a toplevel surface */
    // TODO: Rework this.
    if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        struct wlr_xdg_surface* parent =
            wlr_xdg_surface_from_wlr_surface(wlr_xdg_surface->popup->parent);
        struct wlr_scene_node* parent_node = parent->data;
        /* Create a scene node for the newly created wlr_xdg_surface, and attach
         * it to its parent, so it renders. */
        wlr_xdg_surface->data =
            wlr_scene_xdg_surface_create(parent_node, wlr_xdg_surface);
    } else if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        struct bsi_view* bsi_view = calloc(1, sizeof(struct bsi_view));
        bsi_view_init(bsi_view, bsi_server, wlr_xdg_surface);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_DESTROY_XDG_SURFACE,
                              &bsi_view->events.destroy_xdg_surface,
                              &bsi_view->wlr_xdg_surface->events.destroy,
                              bsi_view_destroy_xdg_surface_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_PING_TIMEOUT,
                              &bsi_view->events.ping_timeout,
                              &bsi_view->wlr_xdg_surface->events.ping_timeout,
                              bsi_view_ping_timeout_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_NEW_POPUP,
                              &bsi_view->events.new_popup,
                              &bsi_view->wlr_xdg_surface->events.new_popup,
                              bsi_view_new_popup_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_MAP,
                              &bsi_view->events.map,
                              &bsi_view->wlr_xdg_surface->events.map,
                              bsi_view_map_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_UNMAP,
                              &bsi_view->events.unmap,
                              &bsi_view->wlr_xdg_surface->events.unmap,
                              bsi_view_unmap_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_CONFIGURE,
                              &bsi_view->events.configure,
                              &bsi_view->wlr_xdg_surface->events.configure,
                              bsi_view_configure_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_ACK_CONFIGURE,
                              &bsi_view->events.ack_configure,
                              &bsi_view->wlr_xdg_surface->events.ack_configure,
                              bsi_view_ack_configure_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_DESTROY_SCENE_NODE,
                              &bsi_view->events.destroy_scene_node,
                              &bsi_view->wlr_scene_node->events.destroy,
                              bsi_view_destroy_scene_node_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_REQUEST_MAXIMIZE,
            &bsi_view->events.request_maximize,
            &bsi_view->wlr_xdg_surface->toplevel->events.request_maximize,
            bsi_view_request_maximize_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_REQUEST_FULLSCREEN,
            &bsi_view->events.request_fullscreen,
            &bsi_view->wlr_xdg_surface->toplevel->events.request_fullscreen,
            bsi_view_request_fullscreen_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_REQUEST_MINIMIZE,
            &bsi_view->events.request_minimize,
            &bsi_view->wlr_xdg_surface->toplevel->events.request_minimize,
            bsi_view_request_minimize_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_REQUEST_MOVE,
            &bsi_view->events.request_move,
            &bsi_view->wlr_xdg_surface->toplevel->events.request_move,
            bsi_view_request_move_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_REQUEST_RESIZE,
            &bsi_view->events.request_resize,
            &bsi_view->wlr_xdg_surface->toplevel->events.request_resize,
            bsi_view_request_resize_notify);
        bsi_view_add_listener(bsi_view,
                              BSI_VIEW_LISTENER_REQUEST_SHOW_WINDOW_MENU,
                              &bsi_view->events.request_show_window_menu,
                              &bsi_view->wlr_xdg_surface->toplevel->events
                                   .request_show_window_menu,
                              bsi_view_request_show_window_menu_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_SET_PARENT,
            &bsi_view->events.set_parent,
            &bsi_view->wlr_xdg_surface->toplevel->events.set_parent,
            bsi_view_set_parent_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_SET_TITLE,
            &bsi_view->events.set_title,
            &bsi_view->wlr_xdg_surface->toplevel->events.set_title,
            bsi_view_set_title_notify);
        bsi_view_add_listener(
            bsi_view,
            BSI_VIEW_LISTENER_SET_APP_ID,
            &bsi_view->events.set_app_id,
            &bsi_view->wlr_xdg_surface->toplevel->events.set_app_id,
            bsi_view_set_app_id_notify);
    } else {
        wlr_log(WLR_DEBUG,
                "Got unsupported wlr_xdg_surface from client: type %d, "
                "discarding event",
                wlr_xdg_surface->role);
    }
}

void
bsi_listeners_xdg_shell_destroy_notify(
    __attribute__((unused)) struct wl_listener* listener,
    __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got event destroy from wlr_xdg_shell");
#warning "Not implemented"
}
