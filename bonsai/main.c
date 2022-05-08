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
#include "bonsai/scene.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

// TODO: Figure out a sensible place to put different listener implementations

static void
bsi_output_destroy_notify(struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
    struct bsi_output* bsi_output =
        wl_container_of(listener, bsi_output, events.destroy);

    bsi_outputs_remove(&bsi_output->bsi_server->bsi_outputs, bsi_output);
}

static void
bsi_output_frame_notify(struct wl_listener* listener,
                        __attribute((unused)) void* data)
{
    struct bsi_output* bsi_output =
        wl_container_of(listener, bsi_output, events.frame);
    struct wlr_scene* wlr_scene = bsi_output->bsi_server->wlr_scene;

    struct wlr_scene_output* wlr_scene_output =
        wlr_scene_get_scene_output(wlr_scene, bsi_output->wlr_output);

    wlr_scene_output_commit(wlr_scene_output);

    struct timespec now = bsi_util_timespec_get();
    wlr_scene_output_send_frame_done(wlr_scene_output, &now);
}

static void
bsi_listeners_new_output_notify(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "Got new_output event from wlr_backend");

    struct wlr_output* wlr_output = data;
    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_backend.new_output);

    wlr_output_init_render(
        wlr_output, bsi_server->wlr_allocator, bsi_server->wlr_renderer);

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

    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_DESTROY,
                            &bsi_output->events.destroy,
                            &bsi_output->wlr_output->events.destroy,
                            bsi_output_destroy_notify);

    bsi_output_add_listener(bsi_output,
                            BSI_OUTPUT_LISTENER_FRAME,
                            &bsi_output->events.frame,
                            &bsi_output->wlr_output->events.frame,
                            bsi_output_frame_notify);

    bsi_outputs_add(&bsi_server->bsi_outputs, bsi_output);
    wlr_output_layout_add_auto(bsi_server->wlr_output_layout, wlr_output);
}

static void
bsi_listeners_new_input_notify(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "Got new_input event from wlr_backend");

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

static void
bsi_listeners_destroy_notify(struct wl_listener* listener,
                             __attribute__((unused)) void* data)
{
    wlr_log(WLR_DEBUG, "Got destroy event from wlr_backend, exiting");

    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_backend.destroy);

    // TODO: Add cleanup functions for other server members.

    wlr_renderer_destroy(bsi_server->wlr_renderer);
    wlr_allocator_destroy(bsi_server->wlr_allocator);
    wlr_output_layout_destroy(bsi_server->wlr_output_layout);
    wlr_seat_destroy(bsi_server->wlr_seat);
    wlr_cursor_destroy(bsi_server->wlr_cursor);
    wlr_xcursor_manager_destroy(bsi_server->wlr_xcursor_manager);
    wl_display_destroy_clients(bsi_server->wl_display);
    wl_display_destroy(bsi_server->wl_display);

    exit(EXIT_SUCCESS);
}

static void
bsi_listeners_new_xdg_surface_notify(struct wl_listener* listener, void* data)
{
    wlr_log(WLR_DEBUG, "Got new_surface event from wlr_xdg_shell");

    struct wlr_xdg_surface* wlr_xdg_surface = data;
    struct bsi_server* bsi_server = wl_container_of(
        listener, bsi_server, bsi_listeners.wlr_xdg_shell.new_surface);

    /* Firstly check if wlr_xdg_surface is a popup surface. If it is not a popup
     * surface, then it is a toplevel surface */
    if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        struct wlr_xdg_surface* parent =
            wlr_xdg_surface_from_wlr_surface(wlr_xdg_surface->popup->parent);
        struct wlr_scene_node* parent_node = parent->data;
        wlr_xdg_surface->data =
            wlr_scene_xdg_surface_create(parent_node, wlr_xdg_surface);
    } else if (wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        struct bsi_view* bsi_view = calloc(1, sizeof(struct bsi_view));
        bsi_view->bsi_server = bsi_server;
        bsi_view->wlr_xdg_surface = wlr_xdg_surface;
        bsi_view->active_listeners = 0;
        bsi_view->len_active_links = 0;
        /* Create a new node from the root server node. */
        bsi_view->wlr_scene_node = wlr_scene_xdg_surface_create(
            &bsi_view->bsi_server->wlr_scene->node, bsi_view->wlr_xdg_surface);
        bsi_view->wlr_scene_node->data = bsi_view;
        wlr_xdg_surface->data = bsi_view->wlr_scene_node;
        // TODO: Add event handlers.
    } else {
        wlr_log(WLR_DEBUG,
                "Got unsupported wlr_xdg_surface from client: type %d",
                wlr_xdg_surface->role);
        exit(EXIT_FAILURE);
    }
}

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

    struct bsi_listeners bsi_listeners;
    bsi_listeners_init(&bsi_listeners);
    server.bsi_listeners = bsi_listeners;

    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_BACKEND_NEW_OUTPUT,
                               &server.bsi_listeners.wlr_backend.new_output,
                               &server.wlr_backend->events.new_output,
                               bsi_listeners_new_output_notify);
    wlr_log(WLR_DEBUG, "Attached bsi_new_output_notify listener");

    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_BACKEND_NEW_INPUT,
                               &server.bsi_listeners.wlr_backend.new_input,
                               &server.wlr_backend->events.new_input,
                               bsi_listeners_new_input_notify);
    wlr_log(WLR_DEBUG, "Attached bsi_new_input_notify listener");

    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_BACKEND_DESTROY,
                               &server.bsi_listeners.wlr_backend.destroy,
                               &server.wlr_backend->events.destroy,
                               bsi_listeners_destroy_notify);
    wlr_log(WLR_DEBUG, "Attached bsi_destroy_notify listener");

    bsi_listeners_add_listener(&server.bsi_listeners,
                               BSI_LISTENERS_XDG_SHELL_NEW_SURFACE,
                               &server.bsi_listeners.wlr_xdg_shell.new_surface,
                               &server.wlr_xdg_shell->events.new_surface,
                               bsi_listeners_new_xdg_surface_notify);
    wlr_log(WLR_DEBUG, "Attached bsi_new_xdg_surface_notify listener");

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
