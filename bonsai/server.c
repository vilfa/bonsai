#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_server*
bsi_server_init(struct bsi_server* server)
{
    assert(server);

    bsi_server_output_init(server);
    bsi_debug("Initialized bsi_outputs");

    server->wl_display = wl_display_create();
    bsi_debug("Created display");

    server->wlr_backend = wlr_backend_autocreate(server->wl_display);
    bsi_util_slot_connect(&server->wlr_backend->events.new_output,
                          &server->listen.new_output,
                          handle_new_output);
    bsi_util_slot_connect(&server->wlr_backend->events.new_input,
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
    bsi_debug(
        "Created wlr_compositor, wlr_subcompositor & wlr_data_device_manager");

    server->wlr_output_layout = wlr_output_layout_create();
    bsi_util_slot_connect(&server->wlr_output_layout->events.change,
                          &server->listen.output_layout_change,
                          handle_output_layout_change);
    bsi_debug("Created output layout");

    server->wlr_output_manager =
        wlr_output_manager_v1_create(server->wl_display);
    bsi_util_slot_connect(&server->wlr_output_manager->events.apply,
                          &server->listen.output_manager_apply,
                          handle_output_manager_apply);
    bsi_util_slot_connect(&server->wlr_output_manager->events.test,
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
    bsi_util_slot_connect(&server->wlr_xdg_shell->events.new_surface,
                          &server->listen.xdg_new_surface,
                          handle_xdgshell_new_surface);
    bsi_debug("Created wlr_xdg_shell & attached handlers");

    server->wlr_server_decoration_manager =
        wlr_server_decoration_manager_create(server->wl_display);
    wlr_server_decoration_manager_set_default_mode(
        server->wlr_server_decoration_manager,
        WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);
    bsi_util_slot_connect(
        &server->wlr_server_decoration_manager->events.new_decoration,
        &server->listen.new_decoration,
        handle_deco_manager_new_decoration);

    bsi_debug("Created wlr_server_decoration_manager & attached handlers");

    server->wlr_layer_shell = wlr_layer_shell_v1_create(server->wl_display);
    bsi_util_slot_connect(&server->wlr_layer_shell->events.new_surface,
                          &server->listen.layer_new_surface,
                          handle_layer_shell_new_surface);
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

    bsi_server_cursor_init(server);
    bsi_debug("Initialized bsi_cursor");

    const char* seat_name = "seat0";
    server->wlr_seat = wlr_seat_create(server->wl_display, seat_name);
    bsi_debug("Created seat '%s'", seat_name);

    bsi_server_input_init(server);
    bsi_debug("Initialized bsi_inputs");

    bsi_util_slot_connect(&server->wlr_seat->events.pointer_grab_begin,
                          &server->listen.pointer_grab_begin,
                          handle_pointer_grab_begin_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.pointer_grab_end,
                          &server->listen.pointer_grab_end,
                          handle_pointer_grab_end_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.keyboard_grab_begin,
                          &server->listen.keyboard_grab_begin,
                          handle_keyboard_grab_begin_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.keyboard_grab_end,
                          &server->listen.keyboard_grab_end,
                          handle_keyboard_grab_end_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.touch_grab_begin,
                          &server->listen.touch_grab_begin,
                          handle_touch_grab_begin_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.touch_grab_end,
                          &server->listen.touch_grab_end,
                          handle_touch_grab_end_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.request_set_cursor,
                          &server->listen.request_set_cursor,
                          handle_request_set_cursor_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.request_set_selection,
                          &server->listen.request_set_selection,
                          handle_request_set_selection_notify);
    bsi_util_slot_connect(
        &server->wlr_seat->events.request_set_primary_selection,
        &server->listen.request_set_primary_selection,
        handle_request_set_primary_selection_notify);
    bsi_util_slot_connect(&server->wlr_seat->events.request_start_drag,
                          &server->listen.request_start_drag,
                          handle_request_start_drag_notify);
    bsi_debug("Attached handlers for seat '%s'", seat_name);

    bsi_server_scene_init(server);
    bsi_debug("Initialized bsi_views");

    server->active_workspace = NULL;
    server->shutting_down = false;

    return server;
}

void
bsi_server_output_init(struct bsi_server* server)
{
    server->output.len = 0;
    wl_list_init(&server->output.outputs);
}

void
bsi_server_input_init(struct bsi_server* server)
{
    server->input.len_pointers = 0;
    server->input.len_keyboards = 0;
    wl_list_init(&server->input.pointers);
    wl_list_init(&server->input.keyboards);
}

void
bsi_server_scene_init(struct bsi_server* server)
{
    server->scene.len_views = 0;
    wl_list_init(&server->scene.views);
    server->scene.len_decorations = 0;
    wl_list_init(&server->scene.decorations);
}

void
bsi_server_cursor_init(struct bsi_server* server)
{
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
}

void
bsi_server_exit(struct bsi_server* server)
{
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
    /* bsi_workspace */
    // wl_list_remove(&server->listen.workspace_active.link);

    wl_display_terminate(server->wl_display);
}
