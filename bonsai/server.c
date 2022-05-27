#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"
#include "bonsai/global.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_server*
bsi_server_init(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    struct bsi_listeners_global bsi_listeners;
    bsi_listeners_global_init(&bsi_listeners, bsi_server);
    bsi_server->bsi_listeners_global = bsi_listeners;
    wlr_log(WLR_DEBUG, "Initialized bsi_listeners_global");

    struct bsi_outputs bsi_outputs;
    bsi_outputs_init(&bsi_outputs, bsi_server);
    bsi_server->bsi_outputs = bsi_outputs;
    wlr_log(WLR_DEBUG, "Initialized bsi_outputs");

    bsi_server->wl_display = wl_display_create();
    wlr_log(WLR_DEBUG, "Created display");

    bsi_server->wlr_backend = wlr_backend_autocreate(bsi_server->wl_display);
    bsi_util_slot_connect(
        &bsi_server->wlr_backend->events.new_output,
        &bsi_server->bsi_listeners_global.listen.wlr_backend_new_output,
        bsi_global_backend_new_output_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_backend->events.new_input,
        &bsi_server->bsi_listeners_global.listen.wlr_backend_new_input,
        bsi_global_backend_new_input_notify);
    wlr_log(WLR_DEBUG, "Autocreated backend & attached handlers");

    bsi_server->wlr_renderer = wlr_renderer_autocreate(bsi_server->wlr_backend);
    if (!wlr_renderer_init_wl_display(bsi_server->wlr_renderer,
                                      bsi_server->wl_display)) {
        wlr_log(WLR_ERROR, "Failed to intitialize renderer with wl_display");
        wlr_backend_destroy(bsi_server->wlr_backend);
        wl_display_destroy(bsi_server->wl_display);
        exit(EXIT_FAILURE);
    }
    wlr_log(WLR_DEBUG, "Autocreated renderer & initialized wl_display");

    bsi_server->wlr_allocator = wlr_allocator_autocreate(
        bsi_server->wlr_backend, bsi_server->wlr_renderer);
    wlr_log(WLR_DEBUG, "Autocreated wlr_allocator");

    wlr_compositor_create(bsi_server->wl_display, bsi_server->wlr_renderer);
    wlr_subcompositor_create(bsi_server->wl_display);
    wlr_data_device_manager_create(bsi_server->wl_display);
    wlr_log(
        WLR_DEBUG,
        "Created wlr_compositor, wlr_subcompositor & wlr_data_device_manager");

    bsi_server->wlr_output_layout = wlr_output_layout_create();
    wlr_log(WLR_DEBUG, "Created output layout");

    bsi_server->wlr_scene = wlr_scene_create();
    wlr_scene_attach_output_layout(bsi_server->wlr_scene,
                                   bsi_server->wlr_output_layout);
    wlr_log(WLR_DEBUG, "Created wlr_scene & attached wlr_output_layout");

    bsi_server->wlr_xdg_shell = wlr_xdg_shell_create(bsi_server->wl_display, 2);
    bsi_util_slot_connect(
        &bsi_server->wlr_xdg_shell->events.new_surface,
        &bsi_server->bsi_listeners_global.listen.wlr_xdg_shell_new_surface,
        bsi_global_xdg_shell_new_surface_notify);
    wlr_log(WLR_DEBUG, "Created wlr_xdg_shell & attached handlers");

    bsi_server->wlr_layer_shell =
        wlr_layer_shell_v1_create(bsi_server->wl_display);
    bsi_util_slot_connect(
        &bsi_server->wlr_layer_shell->events.new_surface,
        &bsi_server->bsi_listeners_global.listen.wlr_layer_shell_new_surface,
        bsi_layer_shell_new_surface_notify);
    wlr_log(WLR_DEBUG, "Created wlr_layer_shell_v1 & attached handlers");

    bsi_server->wlr_cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(bsi_server->wlr_cursor,
                                    bsi_server->wlr_output_layout);
    wlr_log(WLR_DEBUG, "Created wlr_cursor & attached it to wlr_output_layout");

    const float cursor_scale = 1.0f;
    bsi_server->wlr_xcursor_manager = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(bsi_server->wlr_xcursor_manager, cursor_scale);
    wlr_log(
        WLR_DEBUG,
        "Created wlr_xcursor_manager & loaded xcursor theme with scale %.1f",
        cursor_scale);

    struct bsi_cursor bsi_cursor;
    bsi_cursor_init(&bsi_cursor, bsi_server);
    bsi_server->bsi_cursor = bsi_cursor;
    wlr_log(WLR_DEBUG, "Initialized bsi_cursor");

    const char* seat_name = "seat0";
    bsi_server->wlr_seat = wlr_seat_create(bsi_server->wl_display, seat_name);
    wlr_log(WLR_DEBUG, "Created seat '%s'", seat_name);

    struct bsi_inputs bsi_inputs;
    bsi_inputs_init(&bsi_inputs, bsi_server);
    bsi_server->bsi_inputs = bsi_inputs;
    wlr_log(WLR_DEBUG, "Initialized bsi_inputs");

    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.pointer_grab_begin,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_pointer_grab_begin,
        bsi_global_seat_pointer_grab_begin_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.pointer_grab_end,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_pointer_grab_end,
        bsi_global_seat_pointer_grab_end_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.keyboard_grab_begin,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_keyboard_grab_begin,
        bsi_global_seat_keyboard_grab_begin_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.keyboard_grab_end,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_keyboard_grab_end,
        bsi_global_seat_keyboard_grab_end_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.touch_grab_begin,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_touch_grab_begin,
        bsi_global_seat_touch_grab_begin_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.touch_grab_end,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_touch_grab_end,
        bsi_global_seat_touch_grab_end_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.request_set_cursor,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_request_set_cursor,
        bsi_global_seat_request_set_cursor_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.request_set_selection,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_request_set_selection,
        bsi_global_seat_request_set_selection_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.request_set_primary_selection,
        &bsi_server->bsi_listeners_global.listen
             .wlr_seat_request_set_primary_selection,
        bsi_global_seat_request_set_primary_selection_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.request_start_drag,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_request_start_drag,
        bsi_global_seat_request_start_drag_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.start_drag,
        &bsi_server->bsi_listeners_global.listen.wlr_seat_start_drag,
        bsi_global_seat_start_drag_notify);
    wlr_log(WLR_DEBUG, "Attached handlers for seat '%s'", seat_name);

    struct bsi_views bsi_views;
    bsi_views_init(&bsi_views, bsi_server);
    bsi_server->bsi_views = bsi_views;
    wlr_log(WLR_DEBUG, "Initialized bsi_views");

    return bsi_server;
}

void
bsi_server_exit(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    bsi_listeners_global_finish(&bsi_server->bsi_listeners_global);

    wl_display_terminate(bsi_server->wl_display);
}
