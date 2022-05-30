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
#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_server*
bsi_server_init(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    bsi_server_output_init(bsi_server);
    bsi_debug("Initialized bsi_outputs");

    bsi_server->wl_display = wl_display_create();
    bsi_debug("Created display");

    bsi_server->wlr_backend = wlr_backend_autocreate(bsi_server->wl_display);
    bsi_util_slot_connect(&bsi_server->wlr_backend->events.new_output,
                          &bsi_server->listen.backend_new_output,
                          bsi_global_backend_new_output_notify);
    bsi_util_slot_connect(&bsi_server->wlr_backend->events.new_input,
                          &bsi_server->listen.backend_new_input,
                          bsi_global_backend_new_input_notify);
    bsi_debug("Autocreated backend & attached handlers");

    bsi_server->wlr_renderer = wlr_renderer_autocreate(bsi_server->wlr_backend);
    if (!wlr_renderer_init_wl_display(bsi_server->wlr_renderer,
                                      bsi_server->wl_display)) {
        bsi_error("Failed to intitialize renderer with wl_display");
        wlr_backend_destroy(bsi_server->wlr_backend);
        wl_display_destroy(bsi_server->wl_display);
        exit(EXIT_FAILURE);
    }
    bsi_debug("Autocreated renderer & initialized wl_display");

    bsi_server->wlr_allocator = wlr_allocator_autocreate(
        bsi_server->wlr_backend, bsi_server->wlr_renderer);
    bsi_debug("Autocreated wlr_allocator");

    wlr_compositor_create(bsi_server->wl_display, bsi_server->wlr_renderer);
    wlr_subcompositor_create(bsi_server->wl_display);
    wlr_data_device_manager_create(bsi_server->wl_display);
    bsi_debug(
        "Created wlr_compositor, wlr_subcompositor & wlr_data_device_manager");

    bsi_server->wlr_output_layout = wlr_output_layout_create();
    bsi_debug("Created output layout");

    bsi_server->wlr_scene = wlr_scene_create();
    wlr_scene_attach_output_layout(bsi_server->wlr_scene,
                                   bsi_server->wlr_output_layout);
    bsi_debug("Created wlr_scene & attached wlr_output_layout");

    bsi_server->wlr_xdg_shell = wlr_xdg_shell_create(bsi_server->wl_display, 2);
    bsi_util_slot_connect(&bsi_server->wlr_xdg_shell->events.new_surface,
                          &bsi_server->listen.xdg_shell_new_surface,
                          bsi_global_xdg_shell_new_surface_notify);
    bsi_debug("Created wlr_xdg_shell & attached handlers");

    bsi_server->wlr_layer_shell =
        wlr_layer_shell_v1_create(bsi_server->wl_display);
    bsi_util_slot_connect(&bsi_server->wlr_layer_shell->events.new_surface,
                          &bsi_server->listen.layer_shell_new_surface,
                          bsi_layer_shell_new_surface_notify);
    bsi_debug("Created wlr_layer_shell_v1 & attached handlers");

    bsi_server->wlr_cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(bsi_server->wlr_cursor,
                                    bsi_server->wlr_output_layout);
    bsi_debug("Created wlr_cursor & attached it to wlr_output_layout");

    const float cursor_scale = 1.0f;
    bsi_server->wlr_xcursor_manager = wlr_xcursor_manager_create("default", 24);
    wlr_xcursor_manager_load(bsi_server->wlr_xcursor_manager, cursor_scale);
    bsi_debug(
        "Created wlr_xcursor_manager & loaded xcursor theme with scale %.1f",
        cursor_scale);

    bsi_server_cursor_init(bsi_server);
    bsi_debug("Initialized bsi_cursor");

    const char* seat_name = "seat0";
    bsi_server->wlr_seat = wlr_seat_create(bsi_server->wl_display, seat_name);
    bsi_debug("Created seat '%s'", seat_name);

    bsi_server_input_init(bsi_server);
    bsi_debug("Initialized bsi_inputs");

    bsi_util_slot_connect(&bsi_server->wlr_seat->events.pointer_grab_begin,
                          &bsi_server->listen.seat_pointer_grab_begin,
                          bsi_global_seat_pointer_grab_begin_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.pointer_grab_end,
                          &bsi_server->listen.seat_pointer_grab_end,
                          bsi_global_seat_pointer_grab_end_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.keyboard_grab_begin,
                          &bsi_server->listen.seat_keyboard_grab_begin,
                          bsi_global_seat_keyboard_grab_begin_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.keyboard_grab_end,
                          &bsi_server->listen.seat_keyboard_grab_end,
                          bsi_global_seat_keyboard_grab_end_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.touch_grab_begin,
                          &bsi_server->listen.seat_touch_grab_begin,
                          bsi_global_seat_touch_grab_begin_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.touch_grab_end,
                          &bsi_server->listen.seat_touch_grab_end,
                          bsi_global_seat_touch_grab_end_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.request_set_cursor,
                          &bsi_server->listen.seat_request_set_cursor,
                          bsi_global_seat_request_set_cursor_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.request_set_selection,
                          &bsi_server->listen.seat_request_set_selection,
                          bsi_global_seat_request_set_selection_notify);
    bsi_util_slot_connect(
        &bsi_server->wlr_seat->events.request_set_primary_selection,
        &bsi_server->listen.wlr_seat_request_set_primary_selection,
        bsi_global_seat_request_set_primary_selection_notify);
    bsi_util_slot_connect(&bsi_server->wlr_seat->events.request_start_drag,
                          &bsi_server->listen.seat_request_start_drag,
                          bsi_global_seat_request_start_drag_notify);
    bsi_debug("Attached handlers for seat '%s'", seat_name);

    bsi_server_scene_init(bsi_server);
    bsi_debug("Initialized bsi_views");

    bsi_server->shutting_down = false;

    return bsi_server;
}

void
bsi_server_output_init(struct bsi_server* bsi_server)
{
    bsi_server->output.len = 0;
    wl_list_init(&bsi_server->output.outputs);
}

void
bsi_server_input_init(struct bsi_server* bsi_server)
{
    bsi_server->input.len_pointers = 0;
    bsi_server->input.len_keyboards = 0;
    wl_list_init(&bsi_server->input.pointers);
    wl_list_init(&bsi_server->input.keyboards);
}

void
bsi_server_scene_init(struct bsi_server* bsi_server)
{
    bsi_server->scene.len = 0;
    wl_list_init(&bsi_server->scene.views);
}

void
bsi_server_cursor_init(struct bsi_server* bsi_server)
{
    bsi_server->cursor.cursor_mode = BSI_CURSOR_NORMAL;
    bsi_server->cursor.cursor_image = BSI_CURSOR_IMAGE_NORMAL;
    bsi_server->cursor.grab_sx = 0.0;
    bsi_server->cursor.grab_sy = 0.0;
    bsi_server->cursor.resize_edges = 0;
    bsi_server->cursor.grab_box.width = 0;
    bsi_server->cursor.grab_box.height = 0;
    bsi_server->cursor.grab_box.x = 0;
    bsi_server->cursor.grab_box.y = 0;
    bsi_server->cursor.grabbed_view = NULL;
}

void
bsi_server_finish(struct bsi_server* bsi_server)
{
    /* wlr_backend */
    wl_list_remove(&bsi_server->listen.backend_new_output.link);
    wl_list_remove(&bsi_server->listen.backend_new_input.link);
    /* wlr_seat */
    wl_list_remove(&bsi_server->listen.seat_pointer_grab_begin.link);
    wl_list_remove(&bsi_server->listen.seat_pointer_grab_end.link);
    wl_list_remove(&bsi_server->listen.seat_keyboard_grab_begin.link);
    wl_list_remove(&bsi_server->listen.seat_keyboard_grab_end.link);
    wl_list_remove(&bsi_server->listen.seat_touch_grab_begin.link);
    wl_list_remove(&bsi_server->listen.seat_touch_grab_end.link);
    wl_list_remove(&bsi_server->listen.seat_request_set_cursor.link);
    wl_list_remove(&bsi_server->listen.seat_request_set_selection.link);
    wl_list_remove(
        &bsi_server->listen.wlr_seat_request_set_primary_selection.link);
    wl_list_remove(&bsi_server->listen.seat_request_start_drag.link);
    /* wlr_xdg_shell */
    wl_list_remove(&bsi_server->listen.xdg_shell_new_surface.link);
    /* bsi_workspace */
    wl_list_remove(&bsi_server->listen.workspace_active.link);
}

void
bsi_server_exit(struct bsi_server* bsi_server)
{
    bsi_server_finish(bsi_server);

    wl_display_terminate(bsi_server->wl_display);
}
