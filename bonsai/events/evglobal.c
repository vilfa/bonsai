/**
 * @file bsi-listeners.c
 * @brief Contains all event handlers for `bsi_listeners`.
 *
 *
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/backend/drm.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>

struct bsi_view;

#include "bonsai/desktop/decoration.h"
#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
handle_new_output(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_output from wlr_backend");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_output);
    struct wlr_output* wlr_output = data;

    wlr_output_init_render(
        wlr_output, server->wlr_allocator, server->wlr_renderer);

    // TODO: Allow user to pick output mode.

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode =
            wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output))
            return;
    }

    struct bsi_output* output = calloc(1, sizeof(struct bsi_output));
    bsi_output_init(output, server, wlr_output);
    bsi_outputs_add(server, output);
    wlr_output->data = output;

    /* Attach a workspace to the output. */
    char workspace_name[25];
    struct bsi_workspace* wspace = calloc(1, sizeof(struct bsi_workspace));
    sprintf(workspace_name, "Workspace %ld", output->wspace.len + 1);
    bsi_workspace_init(wspace, server, output, workspace_name);
    bsi_workspaces_add(output, wspace);

    bsi_info("Attached %s to output %s", wspace->name, output->output->name);

    bsi_util_slot_connect(&output->output->events.frame,
                          &output->listen.frame,
                          handle_output_frame);
    bsi_util_slot_connect(&output->output->events.destroy,
                          &output->listen.destroy,
                          handle_output_destroy);

    struct wlr_output_configuration_v1* config =
        wlr_output_configuration_v1_create();
    struct wlr_output_configuration_v1_head* config_head =
        wlr_output_configuration_head_v1_create(config, output->output);
    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);

    wlr_output_layout_add_auto(server->wlr_output_layout, wlr_output);
}

void
handle_new_input(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_input from wlr_backend");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_input);
    struct wlr_input_device* device = data;

    switch (device->type) {
        case WLR_INPUT_DEVICE_POINTER: {
            struct bsi_input_pointer* bsi_input_pointer =
                calloc(1, sizeof(struct bsi_input_pointer));
            bsi_input_pointer_init(bsi_input_pointer, server, device);
            bsi_inputs_pointer_add(server, bsi_input_pointer);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.motion,
                                  &bsi_input_pointer->listen.motion,
                                  handle_pointer_motion);
            bsi_util_slot_connect(
                &bsi_input_pointer->cursor->events.motion_absolute,
                &bsi_input_pointer->listen.motion_absolute,
                handle_pointer_motion_absolute);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.button,
                                  &bsi_input_pointer->listen.button,
                                  handle_pointer_button);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.axis,
                                  &bsi_input_pointer->listen.axis,
                                  handle_pointer_axis);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.frame,
                                  &bsi_input_pointer->listen.frame,
                                  handle_pointer_frame);
            bsi_util_slot_connect(
                &bsi_input_pointer->cursor->events.swipe_begin,
                &bsi_input_pointer->listen.swipe_begin,
                handle_pointer_swipe_begin);
            bsi_util_slot_connect(
                &bsi_input_pointer->cursor->events.swipe_update,
                &bsi_input_pointer->listen.swipe_update,
                handle_pointer_swipe_update);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.swipe_end,
                                  &bsi_input_pointer->listen.swipe_end,
                                  handle_pointer_swipe_end);
            bsi_util_slot_connect(
                &bsi_input_pointer->cursor->events.pinch_begin,
                &bsi_input_pointer->listen.pinch_begin,
                handle_pointer_pinch_begin);
            bsi_util_slot_connect(
                &bsi_input_pointer->cursor->events.pinch_update,
                &bsi_input_pointer->listen.pinch_update,
                handle_pointer_pinch_update);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.pinch_end,
                                  &bsi_input_pointer->listen.pinch_end,
                                  handle_pointer_pinch_end);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.hold_begin,
                                  &bsi_input_pointer->listen.hold_begin,
                                  handle_pointer_hold_begin);
            bsi_util_slot_connect(&bsi_input_pointer->cursor->events.hold_end,
                                  &bsi_input_pointer->listen.hold_end,
                                  handle_pointer_hold_end);
            bsi_util_slot_connect(&bsi_input_pointer->device->events.destroy,
                                  &bsi_input_pointer->listen.destroy,
                                  handle_input_device_destroy);

            wlr_cursor_attach_input_device(bsi_input_pointer->cursor,
                                           bsi_input_pointer->device);
            bsi_info("Added new pointer input device");
            break;
        }
        case WLR_INPUT_DEVICE_KEYBOARD: {
            struct bsi_input_keyboard* bsi_input_keyboard =
                calloc(1, sizeof(struct bsi_input_keyboard));
            bsi_input_keyboard_init(bsi_input_keyboard, server, device);
            bsi_inputs_keyboard_add(server, bsi_input_keyboard);
            bsi_util_slot_connect(
                &bsi_input_keyboard->device->keyboard->events.key,
                &bsi_input_keyboard->listen.key,
                handle_keyboard_key);
            bsi_util_slot_connect(
                &bsi_input_keyboard->device->keyboard->events.modifiers,
                &bsi_input_keyboard->listen.modifiers,
                handle_keyboard_modifiers);
            bsi_util_slot_connect(&bsi_input_keyboard->device->events.destroy,
                                  &bsi_input_keyboard->listen.destroy,
                                  handle_input_device_destroy);

            bsi_input_keyboard_keymap_set(bsi_input_keyboard,
                                          bsi_input_keyboard_rules,
                                          bsi_input_keyboard_rules_len);

            wlr_keyboard_set_repeat_info(
                bsi_input_keyboard->device->keyboard, 25, 600);

            wlr_seat_set_keyboard(server->wlr_seat,
                                  bsi_input_keyboard->device->keyboard);
            bsi_info("Added new keyboard input device");
            break;
        }
        default:
            bsi_info("Unsupported new input device: type %d", device->type);
            break;
    }

    uint32_t capabilities = 0;
    size_t len_keyboards = 0, len_pointers = 0;
    if ((len_pointers = server->input.len_pointers) > 0) {
        capabilities |= WL_SEAT_CAPABILITY_POINTER;
        bsi_debug("Seat has capability: WL_SEAT_CAPABILITY_POINTER");
    }
    if ((len_keyboards = server->input.len_keyboards) > 0) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
        bsi_debug("Seat has capability: WL_SEAT_CAPABILITY_KEYBOARD");
    }

    bsi_debug("Server now has %ld input pointer devices", len_pointers);
    bsi_debug("Server now has %ld input keyboard devices", len_keyboards);

    wlr_seat_set_capabilities(server->wlr_seat, capabilities);
}

void
handle_output_layout_change(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event change from wlr_output_layout");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_layout_change);

    if (server->shutting_down)
        return;

    struct wlr_output_configuration_v1* config =
        wlr_output_configuration_v1_create();

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link)
    {
        struct wlr_output_configuration_head_v1* config_head =
            wlr_output_configuration_head_v1_create(config, output->output);
        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->wlr_output_layout, output->output, &output_box);
        config_head->state.mode = output->output->current_mode;
        if (!wlr_box_empty(&output_box)) {
            config_head->state.x = output_box.x;
            config_head->state.y = output_box.y;
        }

        bsi_output_surface_damage(output, NULL, true);
    }

    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);
}

void
handle_output_manager_apply(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event apply from wlr_output_manager");
    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_manager_apply);
    struct wlr_output_configuration_v1* config = data;

    // TODO: Disable outputs that need disabling and enable outputs that need
    // enabling.

    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);
    wlr_output_configuration_v1_send_succeeded(config);
    wlr_output_configuration_v1_destroy(config);
}

void
handle_output_manager_test(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event test from wlr_output_manager");
    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_manager_test);
    struct wlr_output_configuration_v1* config = data;

    // TODO: Finish this.

    bool ok = true;
    struct wlr_output_configuration_head_v1* config_head;
    wl_list_for_each(config_head, &config->heads, link)
    {
        struct wlr_output* output = config_head->state.output;
        bsi_debug("Testing output %s", output->name);

        if (!wl_list_empty(&output->modes)) {
            struct wlr_output_mode* preffered_mode =
                wlr_output_preferred_mode(output);
            wlr_output_set_mode(output, preffered_mode);
        }

        if (wlr_output_is_drm(output)) {
            enum wl_output_transform transf =
                wlr_drm_connector_get_panel_orientation(output);
            if (output->transform != transf)
                wlr_output_set_transform(output, transf);
        }

        ok &= wlr_output_test(output);
    }

    if (ok)
        wlr_output_configuration_v1_send_succeeded(config);
    else
        wlr_output_configuration_v1_send_failed(config);

    wlr_output_configuration_v1_destroy(config);
}

void
handle_pointer_grab_begin_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pointer_grab_begin from wlr_seat");
}

void
handle_pointer_grab_end_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event pointer_grab_end from wlr_seat");
}

void
handle_keyboard_grab_begin_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event keyboard_grab_begin from wlr_seat");
}

void
handle_keyboard_grab_end_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event keyboard_grab_end from wlr_seat");
}

void
handle_touch_grab_begin_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event touch_grab_begin from wlr_seat");
    bsi_debug("A touch device has grabbed focus, what the hell!?");
}

void
handle_touch_grab_end_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event touch_grab_end from wlr_seat");
    bsi_debug("A touch device has ended focus grab, what the hell!?");
}

void
handle_request_set_cursor_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_set_cursor from wlr_seat");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.request_set_cursor);
    struct wlr_seat_pointer_request_set_cursor_event* event = data;

    if (wlr_seat_client_validate_event_serial(event->seat_client,
                                              event->serial))
        wlr_cursor_set_surface(server->wlr_cursor,
                               event->surface,
                               event->hotspot_x,
                               event->hotspot_y);
}

void
handle_request_set_selection_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_set_selection from wlr_seat");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.request_set_selection);
    struct wlr_seat_request_set_selection_event* event = data;

    /* This function also validates the event serial. */
    wlr_seat_set_selection(server->wlr_seat, event->source, event->serial);
}

void
handle_request_set_primary_selection_notify(struct wl_listener* listener,
                                            void* data)
{
    bsi_debug("Got event request_set_primary_selection from wlr_seat");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.request_set_primary_selection);
    struct wlr_seat_request_set_primary_selection_event* event = data;

    /* This function also validates the event serial. */
    wlr_seat_set_primary_selection(
        server->wlr_seat, event->source, event->serial);
}

void
handle_request_start_drag_notify(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_start_drag from wlr_seat");
}

void
handle_xdgshell_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from wlr_xdg_shell");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.xdg_new_surface);
    struct wlr_xdg_surface* xdg_surface = data;

    assert(xdg_surface->role != WLR_XDG_SURFACE_ROLE_NONE);

    /* We must add xdg popups to the scene graph so they get rendered. The
     * wlroots scene graph provides a helper for this, but to use it we must
     * provide the proper parent scene node of the xdg popup. To enable this, we
     * always set the user data field of xdg_surfaces to the corresponding scene
     * node. */
    if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        struct wlr_xdg_surface* parent_surface =
            wlr_xdg_surface_from_wlr_surface(xdg_surface->popup->parent);
        struct wlr_scene_node* parent_node = parent_surface->data;
        xdg_surface->data =
            wlr_scene_xdg_surface_create(parent_node, xdg_surface);
    } else {
        struct bsi_view* view = calloc(1, sizeof(struct bsi_view));
        struct bsi_output* output = bsi_outputs_get_active(server);
        struct bsi_workspace* active_wspace = bsi_workspaces_get_active(output);
        bsi_view_init(view, server, xdg_surface->toplevel);

        bsi_util_slot_connect(&view->toplevel->base->events.destroy,
                              &view->listen.destroy,
                              handle_xdg_surf_destroy);
        bsi_util_slot_connect(&view->toplevel->base->events.map,
                              &view->listen.map,
                              handle_xdg_surf_map);
        bsi_util_slot_connect(&view->toplevel->base->events.unmap,
                              &view->listen.unmap,
                              handle_xdg_surf_unmap);

        bsi_util_slot_connect(&view->toplevel->events.request_maximize,
                              &view->listen.request_maximize,
                              handle_toplvl_request_maximize);
        bsi_util_slot_connect(&view->toplevel->events.request_fullscreen,
                              &view->listen.request_fullscreen,
                              handle_toplvl_request_fullscreen);
        bsi_util_slot_connect(&view->toplevel->events.request_minimize,
                              &view->listen.request_minimize,
                              handle_toplvl_request_minimize);
        bsi_util_slot_connect(&view->toplevel->events.request_move,
                              &view->listen.request_move,
                              handle_toplvl_request_move);
        bsi_util_slot_connect(&view->toplevel->events.request_resize,
                              &view->listen.request_resize,
                              handle_toplvl_request_resize);
        bsi_util_slot_connect(&view->toplevel->events.request_show_window_menu,
                              &view->listen.request_show_window_menu,
                              handle_toplvl_request_show_window_menu);

        /* Add wired up view to workspace on the active output. */
        bsi_workspace_view_add(active_wspace, view);
        bsi_info("Attached view to workspace %s", active_wspace->name);
        bsi_info("Workspace %s now has %ld views",
                 active_wspace->name,
                 active_wspace->len_views);
    }
}

void
handle_layer_shell_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from wlr_layer_shell_v1");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.layer_new_surface);
    struct wlr_layer_surface_v1* layer_surface = data;

    struct bsi_output* active_output;
    if (!layer_surface->output) {
        active_output = bsi_outputs_get_active(server);
        struct bsi_workspace* active_wspace =
            bsi_workspaces_get_active(active_output);
        layer_surface->output = active_wspace->output->output;
        layer_surface->output->data = active_output;
    } else {
        active_output = bsi_outputs_find(server, layer_surface->output);
        layer_surface->output->data = active_output;
    }

    struct bsi_layer_surface_toplevel* bsi_layer =
        calloc(1, sizeof(struct bsi_layer_surface_toplevel));
    bsi_layer_surface_toplevel_init(bsi_layer, layer_surface, active_output);
    bsi_util_slot_connect(&layer_surface->events.map,
                          &bsi_layer->listen.map,
                          handle_layershell_toplvl_map);
    bsi_util_slot_connect(&layer_surface->events.unmap,
                          &bsi_layer->listen.unmap,
                          handle_layershell_toplvl_unmap);
    bsi_util_slot_connect(&layer_surface->events.destroy,
                          &bsi_layer->listen.destroy,
                          handle_layershell_toplvl_destroy);
    bsi_util_slot_connect(&layer_surface->events.new_popup,
                          &bsi_layer->listen.new_popup,
                          handle_layershell_toplvl_new_popup);
    bsi_util_slot_connect(&layer_surface->surface->events.new_subsurface,
                          &bsi_layer->listen.new_subsurface,
                          handle_layershell_toplvl_new_subsurface);
    bsi_util_slot_connect(&layer_surface->surface->events.commit,
                          &bsi_layer->listen.commit,
                          handle_layershell_toplvl_commit);
    bsi_layers_add(active_output, bsi_layer, layer_surface->pending.layer);

    /* Overwrite the current state with pending, so we can look up the desired
     * state when arrangeing the surfaces. Then restore state for wlr.*/
    struct wlr_layer_surface_v1_state old = layer_surface->current;
    layer_surface->current = layer_surface->pending;
    bsi_layers_arrange(active_output);
    layer_surface->current = old;
}

void
handle_deco_manager_new_decoration(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_decoration from wlr_decoration_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_decoration);
    struct wlr_server_decoration* deco = data;

    struct bsi_server_decoration* server_deco =
        calloc(1, sizeof(struct bsi_server_decoration));
    bsi_server_decoration_init(server_deco, server, deco);

    bsi_util_slot_connect(&deco->events.destroy,
                          &server_deco->listen.destroy,
                          handle_serverdeco_destroy);
    bsi_util_slot_connect(
        &deco->events.mode, &server_deco->listen.mode, handle_serverdeco_mode);

    bsi_scene_add_decoration(server, server_deco);
}
