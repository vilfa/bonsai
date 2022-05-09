#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/config/input.h"
#include "bonsai/config/output.h"
#include "bonsai/config/signal.h"
#include "bonsai/cursor.h"
#include "bonsai/events.h"
#include "bonsai/scene/view.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

int
main(void)
{
    wlr_log_init(WLR_DEBUG, NULL);

    // TODO: Break initialization into different function(s).

    struct bsi_server server;

    server.wl_display = wl_display_create();
    assert(server.wl_display);
    wlr_log(WLR_DEBUG, "Created display");

    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);
    wlr_log(WLR_DEBUG, "Acquired event loop");

    server.wlr_backend = wlr_backend_autocreate(server.wl_display);
    assert(server.wlr_backend);
    wlr_log(WLR_DEBUG, "Autocreated backend");

    server.wlr_renderer = wlr_renderer_autocreate(server.wlr_backend);
    assert(server.wlr_renderer);
    wlr_log(WLR_DEBUG, "Autocreated renderer");

    if (!wlr_renderer_init_wl_display(server.wlr_renderer, server.wl_display)) {
        wlr_log(WLR_ERROR, "Failed to intitialize wl_display");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }
    wlr_log(WLR_DEBUG, "Initialized wl_display");

    server.wlr_allocator =
        wlr_allocator_autocreate(server.wlr_backend, server.wlr_renderer);
    assert(server.wlr_allocator);
    wlr_log(WLR_DEBUG, "Autocreated wlr_allocator");

    server.wlr_compositor =
        wlr_compositor_create(server.wl_display, server.wlr_renderer);
    assert(server.wlr_compositor);
    wlr_log(WLR_DEBUG, "Created wlr_compositor");

    server.wlr_data_device_manager =
        wlr_data_device_manager_create(server.wl_display);
    assert(server.wlr_data_device_manager);
    wlr_log(WLR_DEBUG, "Created wlr_data_device_manager");

    server.wlr_output_layout = wlr_output_layout_create();
    assert(server.wlr_output_layout);
    wlr_log(WLR_DEBUG, "Created output layout");

    const char* seat_name = "seat0";
    server.wlr_seat = wlr_seat_create(server.wl_display, seat_name);
    assert(server.wlr_seat);
    wlr_log(WLR_DEBUG, "Created seat %s", seat_name);

    server.wlr_scene = wlr_scene_create();
    assert(server.wlr_scene);
    wlr_log(WLR_DEBUG, "Created wlr_scene");

    wlr_scene_attach_output_layout(server.wlr_scene, server.wlr_output_layout);
    wlr_log(WLR_DEBUG, "Attached wlr_output_layout to wlr_scene");

    server.wlr_xdg_shell = wlr_xdg_shell_create(server.wl_display);
    assert(server.wlr_xdg_shell);
    wlr_log(WLR_DEBUG, "Created wlr_xdg_shell");

    server.wlr_cursor = wlr_cursor_create();
    assert(server.wlr_cursor);
    wlr_log(WLR_DEBUG, "Created wlr_cursor");
    wlr_cursor_attach_output_layout(server.wlr_cursor,
                                    server.wlr_output_layout);
    wlr_log(WLR_DEBUG, "Attached wlr_cursor to wlr_output_layout");

    server.wlr_xcursor_manager = wlr_xcursor_manager_create(NULL, 24);
    assert(server.wlr_xcursor_manager);
    wlr_log(WLR_DEBUG, "Created wlr_xcursor_manager");

    const float cursor_scale = 1.0f;
    wlr_xcursor_manager_load(server.wlr_xcursor_manager, cursor_scale);
    wlr_log(
        WLR_DEBUG, "Loaded wlr_xcursor_manager with scale %1.f", cursor_scale);

    struct bsi_outputs bsi_outputs;
    bsi_outputs_init(&bsi_outputs);
    server.bsi_outputs = bsi_outputs;

    struct bsi_inputs bsi_inputs;
    bsi_inputs_init(&bsi_inputs, server.wlr_seat);
    server.bsi_inputs = bsi_inputs;

    struct bsi_views bsi_views;
    bsi_views_init(&bsi_views);
    server.bsi_views = bsi_views;

    struct bsi_cursor bsi_cursor;
    bsi_cursor_init(&bsi_cursor, &server);
    server.bsi_cursor = bsi_cursor;

    struct bsi_listeners bsi_listeners;
    bsi_listeners_init(&bsi_listeners);
    server.bsi_listeners = bsi_listeners;

    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_BACKEND_NEW_OUTPUT,
                               &server.bsi_listeners.wlr_backend.new_output,
                               &server.wlr_backend->events.new_output,
                               bsi_listeners_backend_new_output_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_BACKEND_NEW_INPUT,
                               &server.bsi_listeners.wlr_backend.new_input,
                               &server.wlr_backend->events.new_input,
                               bsi_listeners_backend_new_input_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_BACKEND_DESTROY,
                               &server.bsi_listeners.wlr_backend.destroy,
                               &server.wlr_backend->events.destroy,
                               bsi_listeners_backend_destroy_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_POINTER_GRAB_BEGIN,
        &server.bsi_listeners.wlr_seat.pointer_grab_begin,
        &server.wlr_seat->events.pointer_grab_begin,
        bsi_listeners_seat_pointer_grab_begin_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_POINTER_GRAB_END,
                               &server.bsi_listeners.wlr_seat.pointer_grab_end,
                               &server.wlr_seat->events.pointer_grab_end,
                               bsi_listeners_seat_pointer_grab_end_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_KEYBOARD_GRAB_BEGIN,
        &server.bsi_listeners.wlr_seat.keyboard_grab_begin,
        &server.wlr_seat->events.keyboard_grab_begin,
        bsi_listeners_seat_keyboard_grab_begin_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_KEYBOARD_GRAB_END,
                               &server.bsi_listeners.wlr_seat.keyboard_grab_end,
                               &server.wlr_seat->events.keyboard_grab_end,
                               bsi_listeners_seat_keyboard_grab_end_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_TOUCH_GRAB_BEGIN,
                               &server.bsi_listeners.wlr_seat.touch_grab_begin,
                               &server.wlr_seat->events.touch_grab_begin,
                               bsi_listeners_seat_touch_grab_begin_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_TOUCH_GRAB_END,
                               &server.bsi_listeners.wlr_seat.touch_grab_end,
                               &server.wlr_seat->events.touch_grab_end,
                               bsi_listeners_seat_touch_grab_end_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_REQUEST_SET_CURSOR,
        &server.bsi_listeners.wlr_seat.request_set_cursor,
        &server.wlr_seat->events.request_set_cursor,
        bsi_listeners_seat_request_set_cursor_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_REQUEST_SET_SELECTION,
        &server.bsi_listeners.wlr_seat.request_set_selection,
        &server.wlr_seat->events.request_set_selection,
        bsi_listeners_seat_request_set_selection_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_SET_SELECTION,
                               &server.bsi_listeners.wlr_seat.set_selection,
                               &server.wlr_seat->events.set_selection,
                               bsi_listeners_seat_set_selection_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_REQUEST_SET_PRIMARY_SELECTION,
        &server.bsi_listeners.wlr_seat.request_set_primary_selection,
        &server.wlr_seat->events.request_set_primary_selection,
        bsi_listeners_seat_request_set_primary_selection_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_SET_PRIMARY_SELECTION,
        &server.bsi_listeners.wlr_seat.set_primary_selection,
        &server.wlr_seat->events.set_primary_selection,
        bsi_listeners_seat_set_primary_selection_notify);
    bsi_listeners_add_listener(
        &server.bsi_listeners,
        BSI_LISTENERS_SEAT_REQUEST_START_DRAG,
        &server.bsi_listeners.wlr_seat.request_start_drag,
        &server.wlr_seat->events.request_start_drag,
        bsi_listeners_seat_request_start_drag_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_START_DRAG,
                               &server.bsi_listeners.wlr_seat.start_drag,
                               &server.wlr_seat->events.start_drag,
                               bsi_listeners_seat_start_drag_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_SEAT_DESTROY,
                               &server.bsi_listeners.wlr_seat.destroy,
                               &server.wlr_seat->events.destroy,
                               bsi_listeners_seat_destroy_notify);

    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_XDG_SHELL_NEW_SURFACE,
                               &server.bsi_listeners.wlr_xdg_shell.new_surface,
                               &server.wlr_xdg_shell->events.new_surface,
                               bsi_listeners_xdg_shell_new_surface_notify);
    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_XDG_SHELL_DESTROY,
                               &server.bsi_listeners.wlr_xdg_shell.destroy,
                               &server.wlr_xdg_shell->events.destroy,
                               bsi_listeners_xdg_shell_destroy_notify);

    server.wl_socket = wl_display_add_socket_auto(server.wl_display);
    assert(server.wl_socket);
    wlr_log(WLR_DEBUG, "Created server socket");
    wlr_log(WLR_DEBUG, "Running compositor on socket '%s'", server.wl_socket);

    if (setenv("WAYLAND_DISPLAY", server.wl_socket, true) != 0) {
        wlr_log(WLR_ERROR,
                "Failed to set WAYLAND_DISPLAY env var: %s",
                strerror(errno));
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    if (!wlr_backend_start(server.wlr_backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    wl_display_run(server.wl_display);

    wl_display_destroy_clients(server.wl_display);
    wl_display_destroy(server.wl_display);

    return EXIT_SUCCESS;
}
