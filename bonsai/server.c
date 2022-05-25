#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/config/signal.h"
#include "bonsai/events.h"
#include "bonsai/server.h"

struct bsi_server*
bsi_server_init(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    bsi_server->wl_display = wl_display_create();
    assert(bsi_server->wl_display);
    wlr_log(WLR_DEBUG, "Created display");

    bsi_server->wl_event_loop =
        wl_display_get_event_loop(bsi_server->wl_display);
    assert(bsi_server->wl_event_loop);
    wlr_log(WLR_DEBUG, "Acquired event loop");

    bsi_server->wlr_backend = wlr_backend_autocreate(bsi_server->wl_display);
    assert(bsi_server->wlr_backend);
    wlr_log(WLR_DEBUG, "Autocreated backend");

    bsi_server->wlr_renderer = wlr_renderer_autocreate(bsi_server->wlr_backend);
    assert(bsi_server->wlr_renderer);
    wlr_log(WLR_DEBUG, "Autocreated renderer");

    if (!wlr_renderer_init_wl_display(bsi_server->wlr_renderer,
                                      bsi_server->wl_display)) {
        wlr_log(WLR_ERROR, "Failed to intitialize wl_display");
        wlr_backend_destroy(bsi_server->wlr_backend);
        wl_display_destroy(bsi_server->wl_display);
        exit(EXIT_FAILURE);
    }
    wlr_log(WLR_DEBUG, "Initialized wl_display");

    bsi_server->wlr_allocator = wlr_allocator_autocreate(
        bsi_server->wlr_backend, bsi_server->wlr_renderer);
    assert(bsi_server->wlr_allocator);
    wlr_log(WLR_DEBUG, "Autocreated wlr_allocator");

    bsi_server->wlr_compositor =
        wlr_compositor_create(bsi_server->wl_display, bsi_server->wlr_renderer);
    assert(bsi_server->wlr_compositor);
    wlr_log(WLR_DEBUG, "Created wlr_compositor");

    bsi_server->wlr_data_device_manager =
        wlr_data_device_manager_create(bsi_server->wl_display);
    assert(bsi_server->wlr_data_device_manager);
    wlr_log(WLR_DEBUG, "Created wlr_data_device_manager");

    bsi_server->wlr_output_layout = wlr_output_layout_create();
    assert(bsi_server->wlr_output_layout);
    wlr_log(WLR_DEBUG, "Created output layout");

    const char* seat_name = "seat0";
    bsi_server->wlr_seat = wlr_seat_create(bsi_server->wl_display, seat_name);
    assert(bsi_server->wlr_seat);
    wlr_log(WLR_DEBUG, "Created seat %s", seat_name);

    bsi_server->wlr_scene = wlr_scene_create();
    assert(bsi_server->wlr_scene);
    wlr_log(WLR_DEBUG, "Created wlr_scene");

    wlr_scene_attach_output_layout(bsi_server->wlr_scene,
                                   bsi_server->wlr_output_layout);
    wlr_log(WLR_DEBUG, "Attached wlr_output_layout to wlr_scene");

    bsi_server->wlr_xdg_shell = wlr_xdg_shell_create(bsi_server->wl_display);
    assert(bsi_server->wlr_xdg_shell);
    wlr_log(WLR_DEBUG, "Created wlr_xdg_shell");

    bsi_server->wlr_cursor = wlr_cursor_create();
    assert(bsi_server->wlr_cursor);
    wlr_log(WLR_DEBUG, "Created wlr_cursor");
    wlr_cursor_attach_output_layout(bsi_server->wlr_cursor,
                                    bsi_server->wlr_output_layout);
    wlr_log(WLR_DEBUG, "Attached wlr_cursor to wlr_output_layout");

    bsi_server->wlr_xcursor_manager = wlr_xcursor_manager_create(NULL, 24);
    assert(bsi_server->wlr_xcursor_manager);
    wlr_log(WLR_DEBUG, "Created wlr_xcursor_manager");

    const float cursor_scale = 1.0f;
    wlr_xcursor_manager_load(bsi_server->wlr_xcursor_manager, cursor_scale);
    wlr_log(
        WLR_DEBUG, "Loaded wlr_xcursor_manager with scale %1.f", cursor_scale);

    struct bsi_outputs bsi_outputs;
    bsi_outputs_init(&bsi_outputs, bsi_server);
    bsi_server->bsi_outputs = bsi_outputs;

    struct bsi_inputs bsi_inputs;
    bsi_inputs_init(&bsi_inputs, bsi_server->wlr_seat);
    bsi_server->bsi_inputs = bsi_inputs;

    struct bsi_views bsi_views;
    bsi_views_init(&bsi_views);
    bsi_server->bsi_views = bsi_views;

    struct bsi_cursor bsi_cursor;
    bsi_cursor_init(&bsi_cursor, bsi_server);
    bsi_server->bsi_cursor = bsi_cursor;

    struct bsi_listeners bsi_listeners;
    bsi_listeners_init(&bsi_listeners);
    bsi_server->bsi_listeners = bsi_listeners;

    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_BACKEND_NEW_OUTPUT,
                      &bsi_server->bsi_listeners.wlr_backend.new_output,
                      &bsi_server->wlr_backend->events.new_output,
                      bsi_listeners_backend_new_output_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_BACKEND_NEW_INPUT,
                      &bsi_server->bsi_listeners.wlr_backend.new_input,
                      &bsi_server->wlr_backend->events.new_input,
                      bsi_listeners_backend_new_input_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_BACKEND_DESTROY,
                      &bsi_server->bsi_listeners.wlr_backend.destroy,
                      &bsi_server->wlr_backend->events.destroy,
                      bsi_listeners_backend_destroy_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_POINTER_GRAB_BEGIN,
                      &bsi_server->bsi_listeners.wlr_seat.pointer_grab_begin,
                      &bsi_server->wlr_seat->events.pointer_grab_begin,
                      bsi_listeners_seat_pointer_grab_begin_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_POINTER_GRAB_END,
                      &bsi_server->bsi_listeners.wlr_seat.pointer_grab_end,
                      &bsi_server->wlr_seat->events.pointer_grab_end,
                      bsi_listeners_seat_pointer_grab_end_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_KEYBOARD_GRAB_BEGIN,
                      &bsi_server->bsi_listeners.wlr_seat.keyboard_grab_begin,
                      &bsi_server->wlr_seat->events.keyboard_grab_begin,
                      bsi_listeners_seat_keyboard_grab_begin_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_KEYBOARD_GRAB_END,
                      &bsi_server->bsi_listeners.wlr_seat.keyboard_grab_end,
                      &bsi_server->wlr_seat->events.keyboard_grab_end,
                      bsi_listeners_seat_keyboard_grab_end_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_TOUCH_GRAB_BEGIN,
                      &bsi_server->bsi_listeners.wlr_seat.touch_grab_begin,
                      &bsi_server->wlr_seat->events.touch_grab_begin,
                      bsi_listeners_seat_touch_grab_begin_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_TOUCH_GRAB_END,
                      &bsi_server->bsi_listeners.wlr_seat.touch_grab_end,
                      &bsi_server->wlr_seat->events.touch_grab_end,
                      bsi_listeners_seat_touch_grab_end_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_REQUEST_SET_CURSOR,
                      &bsi_server->bsi_listeners.wlr_seat.request_set_cursor,
                      &bsi_server->wlr_seat->events.request_set_cursor,
                      bsi_listeners_seat_request_set_cursor_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_REQUEST_SET_SELECTION,
                      &bsi_server->bsi_listeners.wlr_seat.request_set_selection,
                      &bsi_server->wlr_seat->events.request_set_selection,
                      bsi_listeners_seat_request_set_selection_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_SET_SELECTION,
                      &bsi_server->bsi_listeners.wlr_seat.set_selection,
                      &bsi_server->wlr_seat->events.set_selection,
                      bsi_listeners_seat_set_selection_notify);
    bsi_listeners_add(
        &bsi_server->bsi_listeners,
        BSI_LISTENERS_SEAT_REQUEST_SET_PRIMARY_SELECTION,
        &bsi_server->bsi_listeners.wlr_seat.request_set_primary_selection,
        &bsi_server->wlr_seat->events.request_set_primary_selection,
        bsi_listeners_seat_request_set_primary_selection_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_SET_PRIMARY_SELECTION,
                      &bsi_server->bsi_listeners.wlr_seat.set_primary_selection,
                      &bsi_server->wlr_seat->events.set_primary_selection,
                      bsi_listeners_seat_set_primary_selection_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_REQUEST_START_DRAG,
                      &bsi_server->bsi_listeners.wlr_seat.request_start_drag,
                      &bsi_server->wlr_seat->events.request_start_drag,
                      bsi_listeners_seat_request_start_drag_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_START_DRAG,
                      &bsi_server->bsi_listeners.wlr_seat.start_drag,
                      &bsi_server->wlr_seat->events.start_drag,
                      bsi_listeners_seat_start_drag_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_SEAT_DESTROY,
                      &bsi_server->bsi_listeners.wlr_seat.destroy,
                      &bsi_server->wlr_seat->events.destroy,
                      bsi_listeners_seat_destroy_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_XDG_SHELL_NEW_SURFACE,
                      &bsi_server->bsi_listeners.wlr_xdg_shell.new_surface,
                      &bsi_server->wlr_xdg_shell->events.new_surface,
                      bsi_listeners_xdg_shell_new_surface_notify);
    bsi_listeners_add(&bsi_server->bsi_listeners,
                      BSI_LISTENERS_XDG_SHELL_DESTROY,
                      &bsi_server->bsi_listeners.wlr_xdg_shell.destroy,
                      &bsi_server->wlr_xdg_shell->events.destroy,
                      bsi_listeners_xdg_shell_destroy_notify);

    return bsi_server;
}

void
bsi_server_exit(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    wl_display_terminate(bsi_server->wl_display);
}
