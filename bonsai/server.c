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

struct bsi_server*
server_init(struct bsi_server* server, struct bsi_config* config)
{
    server->config.config = config;
    server->config.wallpaper = NULL;
    server->config.workspaces = 0;
    wl_list_init(&server->config.input);
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
                      handle_xdg_shell_new_surface);
    bsi_debug("Created wlr_xdg_shell & attached handlers");

    server->wlr_xdg_decoration_manager =
        wlr_xdg_decoration_manager_v1_create(server->wl_display);
    util_slot_connect(
        &server->wlr_xdg_decoration_manager->events.new_toplevel_decoration,
        &server->listen.new_decoration,
        handle_xdg_decoration_manager_new_decoration);
    bsi_debug("Created wlr_xdg_decoration_manager & attached handlers");

    server->wlr_layer_shell = wlr_layer_shell_v1_create(server->wl_display);
    util_slot_connect(&server->wlr_layer_shell->events.new_surface,
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
                      handle_session_lock_manager_new_lock);
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
    util_slot_connect(&server->wlr_seat->events.request_set_cursor,
                      &server->listen.request_set_cursor,
                      handle_request_set_cursor_notify);
    util_slot_connect(&server->wlr_seat->events.request_set_selection,
                      &server->listen.request_set_selection,
                      handle_request_set_selection_notify);
    util_slot_connect(&server->wlr_seat->events.request_set_primary_selection,
                      &server->listen.request_set_primary_selection,
                      handle_request_set_primary_selection_notify);
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
        server->output.setup[i] = false;
    }

    if (!server->config.wallpaper)
        server->config.wallpaper = "assets/Wallpaper-Default.jpg";
    if (!server->config.workspaces)
        server->config.workspaces = 5;

    return server;
}

static char* server_extern_progs[] = {
    [BSI_SERVER_EXTERN_PROG_WALLPAPER] = "swaybg",
    [BSI_SERVER_EXTERN_PROG_BAR] = "waybar",
};

void
server_setup(struct bsi_server* server)
{
    for (size_t i = 0; i < BSI_SERVER_EXTERN_PROG_MAX; ++i) {
        if (!server->output.setup[i]) {
            const char* exep = server_extern_progs[i];
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
            server->output.setup[i] = true;
        }
    }
}

void
server_finish(struct bsi_server* server)
{
    bsi_debug("Server finish");

    server->session.shutting_down = true;
    wl_list_remove(&server->listen.new_output.link);
    wl_list_remove(&server->listen.new_input.link);
    wl_list_remove(&server->listen.pointer_grab_begin.link);
    wl_list_remove(&server->listen.pointer_grab_end.link);
    wl_list_remove(&server->listen.keyboard_grab_begin.link);
    wl_list_remove(&server->listen.keyboard_grab_end.link);
    wl_list_remove(&server->listen.request_set_cursor.link);
    wl_list_remove(&server->listen.request_set_selection.link);
    wl_list_remove(&server->listen.request_set_primary_selection.link);
    wl_list_remove(&server->listen.xdg_new_surface.link);
    config_destroy(server->config.config);
}

/* Outputs */
void
outputs_add(struct bsi_server* server, struct bsi_output* output)
{
    wl_list_insert(&server->output.outputs, &output->link_server);
}

void
outputs_remove(struct bsi_output* output)
{
    wl_list_remove(&output->link_server);
}

struct bsi_output*
outputs_find(struct bsi_server* server, struct wlr_output* wlr_output)
{
    assert(wl_list_length(&server->output.outputs));

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        if (output->output == wlr_output)
            return output;
    }

    return NULL;
}

/* Inputs */
void
inputs_add(struct bsi_server* server, struct bsi_input_device* device)
{
    wl_list_insert(&server->input.inputs, &device->link_server);
}

void
inputs_remove(struct bsi_input_device* device)
{
    wl_list_remove(&device->link_server);
}

/* Workspaces */
void
workspaces_add(struct bsi_output* output, struct bsi_workspace* workspace)
{
    if (wl_list_length(&output->workspaces) > 0) {
        struct bsi_workspace* workspace_active = workspaces_get_active(output);
        workspace_set_active(workspace_active, false);
        workspace_set_active(workspace, true);
    }

    wl_list_insert(&output->workspaces, &workspace->link_output);

    wl_list_insert(&output->server->listen.workspace,
                   &workspace->foreign_listeners[0].link); // bsi_server
    wl_list_insert(&output->listen.workspace,
                   &workspace->foreign_listeners[1].link); // bsi_output

    /* To whom it may concern... */
    util_slot_connect(&workspace->signal.active,
                      &workspace->foreign_listeners[0].active, // bsi_server
                      handle_server_workspace_active);
    util_slot_connect(&workspace->signal.active,
                      &workspace->foreign_listeners[1].active, // bsi_output
                      handle_output_workspace_active);

    workspace_set_active(workspace, true);
}

void
workspaces_remove(struct bsi_output* output, struct bsi_workspace* workspace)
{
    /* Cannot remove the last workspace */
    if (wl_list_length(&output->workspaces) == 1)
        return;

    /* Get the workspace adjacent to this one. */
    struct wl_list* workspace_adj_link;
    if (wl_list_length(&output->workspaces) > (int)workspace->id) {
        workspace_adj_link = output->workspaces.next;
    } else {
        workspace_adj_link = output->workspaces.prev;
    }

    struct bsi_workspace* workspace_adj =
        wl_container_of(workspace_adj_link, workspace_adj, link_output);

    /* Move all the views to adjacent workspace. */
    if (!wl_list_empty(&workspace->views)) {
        struct bsi_view *view, *view_tmp;
        wl_list_for_each_safe(view, view_tmp, &workspace->views, link_workspace)
        {
            workspace_view_move(workspace, workspace_adj, view);
        }
    }

    /* Take care of the workspaces state. */
    workspace_set_active(workspace, false);
    workspace_set_active(workspace_adj, true);
    wl_list_remove(&workspace->foreign_listeners[0].link); // bsi_server
    wl_list_remove(&workspace->foreign_listeners[1].link); // bsi_output
    util_slot_disconnect(&workspace->foreign_listeners[0].active);
    util_slot_disconnect(&workspace->foreign_listeners[1].active);
    wl_list_remove(&workspace->link_output);
}

struct bsi_workspace*
workspaces_get_active(struct bsi_output* output)
{
    struct bsi_workspace* workspace;
    wl_list_for_each(workspace, &output->workspaces, link_output)
    {
        if (workspace->active)
            return workspace;
    }

    /* Should not happen. */
    return NULL;
}

void
workspaces_next(struct bsi_output* output)
{
    bsi_info("Switch to next workspace");

    int32_t len_ws = wl_list_length(&output->workspaces);
    if (len_ws < (int32_t)output->server->config.workspaces &&
        (int32_t)output->active_workspace->id == len_ws - 1) {
        /* Attach a workspace to the output. */
        char workspace_name[25];
        struct bsi_workspace* workspace =
            calloc(1, sizeof(struct bsi_workspace));
        sprintf(workspace_name,
                "Workspace %d",
                wl_list_length(&output->workspaces) + 1);
        workspace_init(workspace, output->server, output, workspace_name);
        workspaces_add(output, workspace);

        bsi_info(
            "Created new workspace %ld/%s", workspace->id, workspace->name);
        bsi_info("Attached %ld/%s to output %ld/%s",
                 workspace->id,
                 workspace->name,
                 output->id,
                 output->output->name);
    } else {
        struct bsi_workspace* next_workspace =
            output_get_next_workspace(output);
        workspace_set_active(output->active_workspace, false);
        workspace_set_active(next_workspace, true);
    }
}

void
workspaces_prev(struct bsi_output* output)
{
    bsi_info("Switch to previous workspace");
    struct bsi_workspace* prev_workspace = output_get_prev_workspace(output);
    workspace_set_active(output->active_workspace, false);
    workspace_set_active(prev_workspace, true);
}

/* Views */
void
views_add(struct bsi_server* server, struct bsi_view* view)
{
    wl_list_insert(&server->scene.views, &view->link_server);
    /* Initialize geometry state and arrange output. */
    wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base, &view->geom);
    wlr_scene_node_coords(view->node, &view->geom.x, &view->geom.y);
    output_layers_arrange(view->workspace->output);
}

void
views_remove(struct bsi_view* view)
{
    wl_list_remove(&view->link_server);
}

void
views_focus_recent(struct bsi_server* server)
{
    struct wlr_output* active_wout =
        wlr_output_layout_output_at(server->wlr_output_layout,
                                    server->wlr_cursor->x,
                                    server->wlr_cursor->y);
    struct bsi_output* active_out = outputs_find(server, active_wout);
    struct bsi_workspace* active_ws = workspaces_get_active(active_out);
    if (!wl_list_empty(&active_ws->views)) {
        struct bsi_view* mru =
            wl_container_of(active_ws->views.prev, mru, link_workspace);
        if (!mru->mapped)
            return;
        wl_list_remove(&mru->link_workspace);
        wl_list_insert(&active_ws->views, &mru->link_workspace);
        view_focus(mru);
    }
}

struct bsi_view*
views_get_focused(struct bsi_server* server)
{
    if (!wl_list_empty(&server->scene.views)) {
        struct bsi_view* focused =
            wl_container_of(server->scene.views.next, focused, link_server);
        return focused;
    }
    return NULL;
}

/* Layers */
void
layers_add(struct bsi_output* output, struct bsi_layer_surface_toplevel* layer)
{
    wl_list_insert(&output->layers[layer->at_layer], &layer->link_output);
}

void
layers_remove(struct bsi_layer_surface_toplevel* layer)
{
    wl_list_remove(&layer->link_output);
}

/* Idle inhibitors */
void
idle_inhibitors_add(struct bsi_server* server,
                    struct bsi_idle_inhibitor* inhibitor)
{
    wl_list_insert(&server->idle.inhibitors, &inhibitor->link_server);
}

void
idle_inhibitors_remove(struct bsi_idle_inhibitor* inhibitor)
{
    wl_list_remove(&inhibitor->link_server);
}

void
idle_inhibitors_update(struct bsi_server* server)
{
    if (wl_list_empty(&server->idle.inhibitors))
        return;

    bool inhibit = false;
    struct bsi_idle_inhibitor* idle;
    wl_list_for_each(idle, &server->idle.inhibitors, link_server)
    {
        if ((inhibit = idle_inhibitor_active(idle)))
            break;
    }

    wlr_idle_set_enabled(server->wlr_idle, NULL, !inhibit);
}

/* Decorations */
void
decorations_add(struct bsi_server* server, struct bsi_xdg_decoration* deco)
{
    wl_list_insert(&server->scene.xdg_decorations, &deco->link_server);
}

void
decorations_remove(struct bsi_xdg_decoration* deco)
{
    wl_list_remove(&deco->link_server);
}

/* Handlers. */
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
