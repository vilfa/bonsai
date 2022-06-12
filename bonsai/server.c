#include <assert.h>
#include <libinput.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/backend/drm.h>
#include <wlr/backend/libinput.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_foreign_registry.h>
#include <wlr/types/wlr_xdg_foreign_v1.h>
#include <wlr/types/wlr_xdg_foreign_v2.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>

#include "bonsai/config/atom.h"
#include "bonsai/config/config.h"
#include "bonsai/desktop/decoration.h"
#include "bonsai/desktop/idle.h"
#include "bonsai/desktop/layers.h"
#include "bonsai/desktop/lock.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

static char* bsi_server_extern_progs[] = { [BSI_SERVER_EXTERN_PROG_WALLPAPER] =
                                               "swaybg",
                                           [BSI_SERVER_EXTERN_PROG_BAR] =
                                               "waybar" };

/* If a new instance should be started for each output. */
static bool bsi_server_extern_progs_per_output[] = {
    [BSI_SERVER_EXTERN_PROG_WALLPAPER] = false,
    [BSI_SERVER_EXTERN_PROG_BAR] = false,
};

struct bsi_server*
server_init(struct bsi_server* server, struct bsi_config* config)
{
    server->config.all = config;
    wl_list_init(&server->config.input);
    server->config.wallpaper = NULL;
    server->config.workspaces_max = 0;

    config_apply(config);

    wl_list_init(&server->output.outputs);
    bsi_debug("Initialized bsi_outputs");

    server->wl_display = wl_display_create();
    bsi_debug("Created display");

    server->wlr_backend = wlr_backend_autocreate(server->wl_display);
    util_slot_connect(&server->wlr_backend->events.new_output,
                      &server->listen.new_output,
                      handle_new_output);
    util_slot_connect(&server->wlr_backend->events.new_input,
                      &server->listen.new_input,
                      handle_new_input);
    bsi_debug("Autocreated backend & attached handlers");

    server->wlr_renderer = wlr_renderer_autocreate(server->wlr_backend);
    if (!wlr_renderer_init_wl_display(server->wlr_renderer,
                                      server->wl_display)) {
        bsi_error("Failed to intitialize renderer with wl_display");
        wlr_backend_destroy(server->wlr_backend);
        wl_display_destroy(server->wl_display);
        exit(EXIT_FAILURE);
    }
    bsi_debug("Autocreated renderer & initialized wl_display");

    server->wlr_allocator =
        wlr_allocator_autocreate(server->wlr_backend, server->wlr_renderer);
    bsi_debug("Autocreated wlr_allocator");

    wlr_compositor_create(server->wl_display, server->wlr_renderer);
    wlr_subcompositor_create(server->wl_display);
    wlr_data_device_manager_create(server->wl_display);
    wlr_gamma_control_manager_v1_create(server->wl_display);
    bsi_debug("Created wlr_compositor, wlr_subcompositor, "
              "wlr_data_device_manager & wlr_gamma_control_manager");

    server->wlr_output_layout = wlr_output_layout_create();
    util_slot_connect(&server->wlr_output_layout->events.change,
                      &server->listen.output_layout_change,
                      handle_output_layout_change);
    bsi_debug("Created output layout");

    server->wlr_output_manager =
        wlr_output_manager_v1_create(server->wl_display);
    util_slot_connect(&server->wlr_output_manager->events.apply,
                      &server->listen.output_manager_apply,
                      handle_output_manager_apply);
    util_slot_connect(&server->wlr_output_manager->events.test,
                      &server->listen.output_manager_test,
                      handle_output_manager_test);
    bsi_debug("Created wlr_output_manager_v1 & attached handlers");

    wlr_xdg_output_manager_v1_create(server->wl_display,
                                     server->wlr_output_layout);
    bsi_debug("Created xdg_output_manager_v1");

    server->wlr_scene = wlr_scene_create();
    wlr_scene_attach_output_layout(server->wlr_scene,
                                   server->wlr_output_layout);
    bsi_debug("Created wlr_scene & attached wlr_output_layout");

    server->wlr_xdg_shell = wlr_xdg_shell_create(server->wl_display, 2);
    util_slot_connect(&server->wlr_xdg_shell->events.new_surface,
                      &server->listen.xdg_new_surface,
                      handle_xdgshell_new_surface);
    bsi_debug("Created wlr_xdg_shell & attached handlers");

    server->wlr_xdg_decoration_manager =
        wlr_xdg_decoration_manager_v1_create(server->wl_display);
    util_slot_connect(
        &server->wlr_xdg_decoration_manager->events.new_toplevel_decoration,
        &server->listen.new_decoration,
        handle_xdg_deco_manager_new_decoration);
    bsi_debug("Created wlr_xdg_decoration_manager & attached handlers");

    server->wlr_layer_shell = wlr_layer_shell_v1_create(server->wl_display);
    util_slot_connect(&server->wlr_layer_shell->events.new_surface,
                      &server->listen.layer_new_surface,
                      handle_layershell_new_surface);
    bsi_debug("Created wlr_layer_shell_v1 & attached handlers");

    server->wlr_cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->wlr_cursor,
                                    server->wlr_output_layout);
    bsi_debug("Created wlr_cursor & attached it to wlr_output_layout");

    const float cursor_scale = 1.0f;
    server->wlr_xcursor_manager = wlr_xcursor_manager_create("default", 24);
    wlr_xcursor_manager_load(server->wlr_xcursor_manager, cursor_scale);
    bsi_debug(
        "Created wlr_xcursor_manager & loaded xcursor theme with scale %.1f",
        cursor_scale);

    wlr_export_dmabuf_manager_v1_create(server->wl_display);
    wlr_screencopy_manager_v1_create(server->wl_display);
    wlr_data_control_manager_v1_create(server->wl_display);
    wlr_primary_selection_v1_device_manager_create(server->wl_display);
    bsi_debug(
        "Created wlr_export_dmabuf_manager, wlr_screencopy_manager, "
        "wlr_data_control_manager & wlr_primary_selection_device_manager");

    struct wlr_xdg_foreign_registry* foreign_registry =
        wlr_xdg_foreign_registry_create(server->wl_display);
    wlr_xdg_foreign_v1_create(server->wl_display, foreign_registry);
    wlr_xdg_foreign_v2_create(server->wl_display, foreign_registry);
    bsi_debug("Created wlr_xdg_foreign_registry");

    server->wlr_xdg_activation =
        wlr_xdg_activation_v1_create(server->wl_display);
    util_slot_connect(&server->wlr_xdg_activation->events.request_activate,
                      &server->listen.request_activate,
                      handle_xdg_request_activate);

    server->wlr_idle = wlr_idle_create(server->wl_display);
    util_slot_connect(&server->wlr_idle->events.activity_notify,
                      &server->listen.activity_notify,
                      handle_idle_activity_notify);
    server->wlr_idle_inhibit_manager =
        wlr_idle_inhibit_v1_create(server->wl_display);
    util_slot_connect(&server->wlr_idle_inhibit_manager->events.new_inhibitor,
                      &server->listen.new_inhibitor,
                      handle_idle_manager_new_inhibitor);
    wl_list_init(&server->idle.inhibitors);
    bsi_debug("Created wlr_idle, wlr_idle_inhibit_manager & added handlers");

    server->wlr_session_lock_manager =
        wlr_session_lock_manager_v1_create(server->wl_display);
    util_slot_connect(&server->wlr_session_lock_manager->events.new_lock,
                      &server->listen.new_lock,
                      handle_session_new_lock);
    server->session.locked = false;
    server->session.lock = NULL;
    bsi_debug("Created wlr_session_lock_manager & added handlers");

    server->cursor.cursor_mode = BSI_CURSOR_NORMAL;
    server->cursor.cursor_image = BSI_CURSOR_IMAGE_NORMAL;
    server->cursor.grab_sx = 0.0;
    server->cursor.grab_sy = 0.0;
    server->cursor.resize_edges = 0;
    server->cursor.grab_box.width = 0;
    server->cursor.grab_box.height = 0;
    server->cursor.grab_box.x = 0;
    server->cursor.grab_box.y = 0;
    server->cursor.grabbed_view = NULL;
    server->cursor.swipe_dx = 0.0;
    server->cursor.swipe_dy = 0.0;
    server->cursor.swipe_cancelled = false;
    server->cursor.swipe_fingers = 0;
    server->cursor.swipe_timest = 0;
    bsi_debug("Initialized bsi_cursor");

    const char* seat_name = "seat0";
    server->wlr_seat = wlr_seat_create(server->wl_display, seat_name);
    bsi_debug("Created seat '%s'", seat_name);

    server->wlr_input_inhbit_manager =
        wlr_input_inhibit_manager_create(server->wl_display);

    wl_list_init(&server->input.inputs);
    bsi_debug("Initialized bsi_inputs");

    util_slot_connect(&server->wlr_seat->events.pointer_grab_begin,
                      &server->listen.pointer_grab_begin,
                      handle_pointer_grab_begin_notify);
    util_slot_connect(&server->wlr_seat->events.pointer_grab_end,
                      &server->listen.pointer_grab_end,
                      handle_pointer_grab_end_notify);
    util_slot_connect(&server->wlr_seat->events.keyboard_grab_begin,
                      &server->listen.keyboard_grab_begin,
                      handle_keyboard_grab_begin_notify);
    util_slot_connect(&server->wlr_seat->events.keyboard_grab_end,
                      &server->listen.keyboard_grab_end,
                      handle_keyboard_grab_end_notify);
    util_slot_connect(&server->wlr_seat->events.touch_grab_begin,
                      &server->listen.touch_grab_begin,
                      handle_touch_grab_begin_notify);
    util_slot_connect(&server->wlr_seat->events.touch_grab_end,
                      &server->listen.touch_grab_end,
                      handle_touch_grab_end_notify);
    util_slot_connect(&server->wlr_seat->events.request_set_cursor,
                      &server->listen.request_set_cursor,
                      handle_request_set_cursor_notify);
    util_slot_connect(&server->wlr_seat->events.request_set_selection,
                      &server->listen.request_set_selection,
                      handle_request_set_selection_notify);
    util_slot_connect(&server->wlr_seat->events.request_set_primary_selection,
                      &server->listen.request_set_primary_selection,
                      handle_request_set_primary_selection_notify);
    util_slot_connect(&server->wlr_seat->events.request_start_drag,
                      &server->listen.request_start_drag,
                      handle_request_start_drag_notify);
    bsi_debug("Attached handlers for seat '%s'", seat_name);

    wl_list_init(&server->scene.views);
    wl_list_init(&server->scene.views_fullscreen);
    wl_list_init(&server->scene.xdg_decorations);
    bsi_debug("Initialized views and decorations");

    wl_list_init(&server->listen.workspace);
    bsi_debug("Initialized workspace listeners");

    server->active_workspace = NULL;
    server->session.shutting_down = false;
    for (size_t i = 0; i < BSI_SERVER_EXTERN_PROG_MAX; ++i) {
        server->output.extern_setup[i] = false;
    }

    if (!server->config.wallpaper)
        server->config.wallpaper = "assets/Wallpaper-Default.jpg";
    if (!server->config.workspaces_max)
        server->config.workspaces_max = 5;

    return server;
}

void
server_setup_extern(struct bsi_server* server)
{
    for (size_t i = 0; i < BSI_SERVER_EXTERN_PROG_MAX; ++i) {
        if (!server->output.extern_setup[i] ||
            bsi_server_extern_progs_per_output[i]) {
            const char* exep = bsi_server_extern_progs[i];
            char* argsp;
            if (i == BSI_SERVER_EXTERN_PROG_WALLPAPER) {
                char swaybg_image[255] = { 0 };
                snprintf(
                    swaybg_image, 255, "--image=%s", server->config.wallpaper);
                argsp = swaybg_image;
            } else {
                argsp = "";
            }
            char** argp = NULL;
            size_t len_argp = util_split_argsp((char*)exep, argsp, " ", &argp);
            util_tryexec(argp, len_argp);
            util_split_free(&argp);
            server->output.extern_setup[i] = true;
        }
    }
}

void
server_finish(struct bsi_server* server)
{
    server->session.shutting_down = true;

    bsi_debug("Server finish");

    /* wlr_backend */
    wl_list_remove(&server->listen.new_output.link);
    wl_list_remove(&server->listen.new_input.link);
    /* wlr_seat */
    wl_list_remove(&server->listen.pointer_grab_begin.link);
    wl_list_remove(&server->listen.pointer_grab_end.link);
    wl_list_remove(&server->listen.keyboard_grab_begin.link);
    wl_list_remove(&server->listen.keyboard_grab_end.link);
    wl_list_remove(&server->listen.touch_grab_begin.link);
    wl_list_remove(&server->listen.touch_grab_end.link);
    wl_list_remove(&server->listen.request_set_cursor.link);
    wl_list_remove(&server->listen.request_set_selection.link);
    wl_list_remove(&server->listen.request_set_primary_selection.link);
    wl_list_remove(&server->listen.request_start_drag.link);
    /* wlr_xdg_shell */
    wl_list_remove(&server->listen.xdg_new_surface.link);

    config_destroy(server->config.all);
}

/**
 * Handlers
 */

void
handle_new_output(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_output from wlr_backend");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_output);
    struct wlr_output* wlr_output = data;

    wlr_output_init_render(
        wlr_output, server->wlr_allocator, server->wlr_renderer);

    /* Allocate output. */
    struct bsi_output* output = calloc(1, sizeof(struct bsi_output));
    output->output = wlr_output;
    outputs_add(server, output);

    /* Configure output. */
    bool has_config = false;
    struct bsi_config_atom* atom;
    wl_list_for_each(atom, &server->config.all->atoms, link)
    {
        if (atom->type == BSI_CONFIG_ATOM_OUTPUT) {
            has_config = atom->impl->apply(atom, server);
        }
    }

    struct wlr_output_mode* preffered_mode =
        wlr_output_preferred_mode(wlr_output);
    /* Set preffered mode first, if output has one. */
    if (preffered_mode && !has_config) {
        bsi_info("Output has preffered mode, setting %dx%d@%d",
                 preffered_mode->width,
                 preffered_mode->height,
                 preffered_mode->refresh);

        wlr_output_set_mode(wlr_output, preffered_mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            bsi_error("Failed to commit on output '%s'", wlr_output->name);
            return;
        }
    } else if (!wl_list_empty(&wlr_output->modes) && !has_config) {
        struct wlr_output_mode* mode =
            wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            bsi_error("Failed to commit on output '%s'", wlr_output->name);
            return;
        }
    }

    output_init(output, server, wlr_output);

    /* Attach a workspace to the output. */
    char workspace_name[25];
    struct bsi_workspace* workspace = calloc(1, sizeof(struct bsi_workspace));
    sprintf(workspace_name,
            "Workspace %d",
            wl_list_length(&output->workspaces) + 1);
    workspace_init(workspace, server, output, workspace_name);
    workspaces_add(output, workspace);

    bsi_info("Attached %s to output %s", workspace->name, output->output->name);

    util_slot_connect(&output->output->events.frame,
                      &output->listen.frame,
                      handle_output_frame);
    util_slot_connect(&output->output->events.destroy,
                      &output->listen.destroy,
                      handle_output_destroy);
    util_slot_connect(&output->damage->events.frame,
                      &output->listen.damage_frame,
                      handle_output_damage_frame);

    struct wlr_output_configuration_v1* config =
        wlr_output_configuration_v1_create();
    struct wlr_output_configuration_head_v1* config_head =
        wlr_output_configuration_head_v1_create(config, output->output);
    wlr_output_manager_v1_set_configuration(server->wlr_output_manager, config);

    wlr_output_layout_add_auto(server->wlr_output_layout, wlr_output);

    /* This if is kinda useless. */
    if (output->new) {
        server_setup_extern(server);
        output->new = false;
    }
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
                              handle_pointer_motion);
            util_slot_connect(&device->cursor->events.motion_absolute,
                              &device->listen.motion_absolute,
                              handle_pointer_motion_absolute);
            util_slot_connect(&device->cursor->events.button,
                              &device->listen.button,
                              handle_pointer_button);
            util_slot_connect(&device->cursor->events.axis,
                              &device->listen.axis,
                              handle_pointer_axis);
            util_slot_connect(&device->cursor->events.frame,
                              &device->listen.frame,
                              handle_pointer_frame);
            util_slot_connect(&device->cursor->events.swipe_begin,
                              &device->listen.swipe_begin,
                              handle_pointer_swipe_begin);
            util_slot_connect(&device->cursor->events.swipe_update,
                              &device->listen.swipe_update,
                              handle_pointer_swipe_update);
            util_slot_connect(&device->cursor->events.swipe_end,
                              &device->listen.swipe_end,
                              handle_pointer_swipe_end);
            util_slot_connect(&device->cursor->events.pinch_begin,
                              &device->listen.pinch_begin,
                              handle_pointer_pinch_begin);
            util_slot_connect(&device->cursor->events.pinch_update,
                              &device->listen.pinch_update,
                              handle_pointer_pinch_update);
            util_slot_connect(&device->cursor->events.pinch_end,
                              &device->listen.pinch_end,
                              handle_pointer_pinch_end);
            util_slot_connect(&device->cursor->events.hold_begin,
                              &device->listen.hold_begin,
                              handle_pointer_hold_begin);
            util_slot_connect(&device->cursor->events.hold_end,
                              &device->listen.hold_end,
                              handle_pointer_hold_end);
            util_slot_connect(&device->device->events.destroy,
                              &device->listen.destroy,
                              handle_input_device_destroy);

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
                              handle_keyboard_key);
            util_slot_connect(&device->device->keyboard->events.modifiers,
                              &device->listen.modifiers,
                              handle_keyboard_modifiers);
            util_slot_connect(&device->device->events.destroy,
                              &device->listen.destroy,
                              handle_input_device_destroy);

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

void
handle_output_layout_change(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event change from wlr_output_layout");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.output_layout_change);

    if (server->session.shutting_down)
        return;

    struct wlr_output_configuration_v1* config =
        wlr_output_configuration_v1_create();

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
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

        /* Reset the usable box. */
        struct wlr_box usable_box = { 0 };
        wlr_output_effective_resolution(
            output->output, &usable_box.width, &usable_box.height);
        output_set_usable_box(output, &usable_box);

        /* Reset the state of the layer shell layers, with regards to output box
         * exclusive configuration. */
        for (size_t i = 0; i < 4; ++i) {
            struct bsi_layer_surface_toplevel* toplevel;
            wl_list_for_each(toplevel, &output->layers[i], link_output)
            {
                toplevel->exclusive_configured = false;
            }
        }

        layers_output_arrange(output);
        output_surface_damage(output, NULL, true);
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
        if (wlr_surface_is_xdg_surface(xdg_surface->popup->parent)) {
            struct wlr_xdg_surface* parent_surface =
                wlr_xdg_surface_from_wlr_surface(xdg_surface->popup->parent);
            struct wlr_scene_node* parent_node = parent_surface->data;
            xdg_surface->data =
                wlr_scene_xdg_surface_create(parent_node, xdg_surface);
        } else if (wlr_surface_is_layer_surface(xdg_surface->popup->parent)) {
            struct wlr_layer_surface_v1* parent_surface =
                wlr_layer_surface_v1_from_wlr_surface(
                    xdg_surface->popup->parent);
            struct bsi_layer_surface_toplevel* parent_layer =
                parent_surface->data;
            struct wlr_scene_node* parent_node = parent_layer->scene_node->node;
            xdg_surface->data =
                wlr_scene_xdg_surface_create(parent_node, xdg_surface);
        }
    } else {
        struct bsi_view* view = calloc(1, sizeof(struct bsi_view));
        struct bsi_output* output =
            wlr_output_layout_output_at(server->wlr_output_layout,
                                        server->wlr_cursor->x,
                                        server->wlr_cursor->y)
                ->data;
        struct bsi_workspace* active_wspace = workspaces_get_active(output);
        view_init(view, server, xdg_surface->toplevel);

        util_slot_connect(&view->toplevel->base->events.destroy,
                          &view->listen.destroy,
                          handle_xdg_surf_destroy);
        util_slot_connect(&view->toplevel->base->events.map,
                          &view->listen.map,
                          handle_xdg_surf_map);
        util_slot_connect(&view->toplevel->base->events.unmap,
                          &view->listen.unmap,
                          handle_xdg_surf_unmap);

        util_slot_connect(&view->toplevel->events.request_maximize,
                          &view->listen.request_maximize,
                          handle_toplvl_request_maximize);
        util_slot_connect(&view->toplevel->events.request_fullscreen,
                          &view->listen.request_fullscreen,
                          handle_toplvl_request_fullscreen);
        util_slot_connect(&view->toplevel->events.request_minimize,
                          &view->listen.request_minimize,
                          handle_toplvl_request_minimize);
        util_slot_connect(&view->toplevel->events.request_move,
                          &view->listen.request_move,
                          handle_toplvl_request_move);
        util_slot_connect(&view->toplevel->events.request_resize,
                          &view->listen.request_resize,
                          handle_toplvl_request_resize);
        util_slot_connect(&view->toplevel->events.request_show_window_menu,
                          &view->listen.request_show_window_menu,
                          handle_toplvl_request_show_window_menu);

        /* Add wired up view to workspace on the active output. */
        workspace_view_add(active_wspace, view);
        bsi_info("Attached view to workspace %s", active_wspace->name);
        bsi_info("Workspace %s now has %d views",
                 active_wspace->name,
                 wl_list_length(&active_wspace->views));
    }
}

void
handle_layershell_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from wlr_layer_shell_v1");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.layer_new_surface);
    struct wlr_layer_surface_v1* layer_surface = data;

    struct bsi_output* active_output;
    if (!layer_surface->output) {
        active_output = wlr_output_layout_output_at(server->wlr_output_layout,
                                                    server->wlr_cursor->x,
                                                    server->wlr_cursor->y)
                            ->data;
        struct bsi_workspace* active_wspace =
            workspaces_get_active(active_output);
        layer_surface->output = active_wspace->output->output;
        layer_surface->output->data = active_output;
    } else {
        active_output = outputs_find(server, layer_surface->output);
        layer_surface->output->data = active_output;
    }

    /* Refuse this client, if a layer is already exclusively taken. */
    // if (!wl_list_empty(&active_output->layers[layer_surface->pending.layer]))
    // {
    //     struct bsi_layer_surface_toplevel* toplevel;
    //     wl_list_for_each(toplevel,
    //                      &active_output->layers[layer_surface->pending.layer],
    //                      link_output)
    //     {
    //         if (toplevel->layer_surface->current.exclusive_zone > 0) {
    //             bsi_debug(
    //                 "Refusing layer shell client wanting already exclusively
    //                 " "taken layer");
    //             bsi_debug("New client namespace '%s', exclusive client "
    //                       "namespace '%s'",
    //                       toplevel->layer_surface->namespace,
    //                       layer_surface->namespace);
    //             wlr_layer_surface_v1_destroy(layer_surface);
    //             return;
    //         }
    //     }
    // }

    struct bsi_layer_surface_toplevel* layer =
        calloc(1, sizeof(struct bsi_layer_surface_toplevel));
    layer_surface_toplevel_init(layer, layer_surface, active_output);
    util_slot_connect(&layer_surface->events.map,
                      &layer->listen.map,
                      handle_layershell_toplvl_map);
    util_slot_connect(&layer_surface->events.unmap,
                      &layer->listen.unmap,
                      handle_layershell_toplvl_unmap);
    util_slot_connect(&layer_surface->events.destroy,
                      &layer->listen.destroy,
                      handle_layershell_toplvl_destroy);
    util_slot_connect(&layer_surface->events.new_popup,
                      &layer->listen.new_popup,
                      handle_layershell_toplvl_new_popup);
    util_slot_connect(&layer_surface->surface->events.new_subsurface,
                      &layer->listen.new_subsurface,
                      handle_layershell_toplvl_new_subsurface);
    util_slot_connect(&layer_surface->surface->events.commit,
                      &layer->listen.commit,
                      handle_layershell_toplvl_commit);

    layers_add(active_output, layer);

    /* Overwrite the current state with pending, so we can look up the
     * desired state when arrangeing the surfaces. Then restore state for
     * wlr.*/
    struct wlr_layer_surface_v1_state old = layer_surface->current;
    layer_surface->current = layer_surface->pending;
    layers_output_arrange(active_output);
    layer_surface->current = old;
}

void
handle_xdg_deco_manager_new_decoration(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_decoration from wlr_decoration_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_decoration);
    struct wlr_xdg_toplevel_decoration_v1* toplevel_deco = data;

    struct bsi_view* view =
        ((struct wlr_scene_node*)toplevel_deco->surface->data)->data;
    assert(view);

    struct bsi_xdg_decoration* xdg_deco =
        calloc(1, sizeof(struct bsi_xdg_decoration));
    decoration_init(xdg_deco, server, view, toplevel_deco);

    util_slot_connect(&toplevel_deco->events.destroy,
                      &xdg_deco->listen.destroy,
                      handle_xdg_decoration_destroy);
    util_slot_connect(&toplevel_deco->events.request_mode,
                      &xdg_deco->listen.request_mode,
                      handle_xdg_decoration_request_mode);

    decorations_add(server, xdg_deco);

    // bsi_decoration_draw(xdg_deco);

    // TODO: Until server side decorations are implemented, refuse to set SSD.
    wlr_xdg_toplevel_decoration_v1_set_mode(
        toplevel_deco, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
    // wlr_xdg_toplevel_decoration_v1_set_mode(toplevel_deco,
    // toplevel_deco->requested_mode);
}

void
handle_xdg_request_activate(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_activate from wlr_xdg_activation");

    struct wlr_xdg_activation_v1_request_activate_event* event = data;

    if (!wlr_surface_is_xdg_surface(event->surface))
        return;

    struct wlr_xdg_surface* surface =
        wlr_xdg_surface_from_wlr_surface(event->surface);

    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    struct wlr_scene_node* node = surface->toplevel->base->data;
    struct bsi_view* view = node->data;

    if (view == NULL || !surface->mapped)
        return;

    view_request_activate(view);
}

void
handle_idle_manager_new_inhibitor(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_inhibitor from wlr_idle_inhibit_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_inhibitor);
    struct wlr_idle_inhibitor_v1* idle_inhibitor = data;

    /* Only xdg and layer shell surfaces can have inhbitors. */
    if (!wlr_surface_is_xdg_surface(idle_inhibitor->surface) &&
        !wlr_surface_is_layer_surface(idle_inhibitor->surface)) {
        bsi_info("Refuse to set inhbitor for non- xdg or layer shell surface");
        return;
    }

    struct bsi_idle_inhibitor* inhibitor =
        calloc(1, sizeof(struct bsi_idle_inhibitor));

    if (wlr_surface_is_xdg_surface(idle_inhibitor->surface)) {
        struct wlr_xdg_surface* xdg_surface =
            wlr_xdg_surface_from_wlr_surface(idle_inhibitor->surface);
        struct bsi_view* view = xdg_surface->data;
        idle_inhibitor_init(inhibitor,
                            idle_inhibitor,
                            server,
                            view,
                            BSI_IDLE_INHIBIT_APPLICATION);
    } else if (wlr_surface_is_layer_surface(idle_inhibitor->surface)) {
        idle_inhibitor_init(
            inhibitor, idle_inhibitor, server, NULL, BSI_IDLE_INHIBIT_USER);
    }

    util_slot_connect(&idle_inhibitor->events.destroy,
                      &inhibitor->listen.destroy,
                      handle_idle_inhibitor_destroy);
    idle_inhibitors_add(server, inhibitor);

    idle_inhibitors_state_update(server);
}

void
handle_idle_activity_notify(struct wl_listener* listener, void* data)
{
    struct bsi_server* server =
        wl_container_of(listener, server, listen.activity_notify);
    idle_inhibitors_state_update(server);
}

void
handle_session_new_lock(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_lock from wlr_session_lock_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_lock);
    struct wlr_session_lock_v1* lock = data;
    // struct wl_client* client = wl_resource_get_client(lock->resource);

    if (server->session.lock != NULL) {
        wlr_session_lock_v1_destroy(lock);
        return;
    }

    server->session.locked = true;
    bsi_info("Session locked");

    struct bsi_session_lock* session_lock =
        calloc(1, sizeof(struct bsi_session_lock));
    session_lock_init(session_lock, server, lock);

    util_slot_connect(&lock->events.new_surface,
                      &session_lock->listen.new_surface,
                      handle_session_lock_new_surface);
    util_slot_connect(&lock->events.unlock,
                      &session_lock->listen.unlock,
                      handle_session_lock_unlock);
    util_slot_connect(&lock->events.destroy,
                      &session_lock->listen.destroy,
                      handle_session_lock_destroy);

    server->session.lock = session_lock;

    wlr_session_lock_v1_send_locked(session_lock->lock);

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}
