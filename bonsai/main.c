#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "bonsai/config/output.h"
#include "bonsai/config/signal.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

static void
bsi_output_destroy_notify(struct wl_listener* listener, void* data)
{
    struct wlr_output* wlr_output = data;
    struct bsi_output* bsi_output =
        wl_container_of(listener, bsi_output, destroy_listener);

    bsi_outputs_remove(&bsi_output->server->bsi_outputs, bsi_output);
}

static void
bsi_output_frame_notify(struct wl_listener* listener, void* data)
{
    struct wlr_output* wlr_output = data;
    struct bsi_output* bsi_output =
        wl_container_of(listener, bsi_output, frame_listener);
    struct wlr_scene* wlr_scene = bsi_output->server->wlr_scene;

    struct wlr_scene_output* wlr_scene_output =
        wlr_scene_get_scene_output(wlr_scene, bsi_output->wlr_output);

    wlr_scene_output_commit(wlr_scene_output);

    struct timespec now = bsi_util_timespec_get();
    wlr_scene_output_send_frame_done(wlr_scene_output, &now);
}

static void
bsi_listeners_new_output_notify(struct wl_listener* listener, void* data)
{
    struct wlr_output* wlr_output = data;
    struct bsi_server* server =
        wl_container_of(listener, server, bsi_listeners.new_output_listener);

    wlr_output_init_render(
        wlr_output, server->wlr_allocator, server->wlr_renderer);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode =
            wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output))
            return;
    }

    struct timespec now = bsi_util_timespec_get();

    struct bsi_output* bsi_output = calloc(1, sizeof(struct bsi_output));
    bsi_output->server = server;
    bsi_output->wlr_output = wlr_output;
    bsi_output->last_frame = now;

    bsi_output_add_destroy_listener(bsi_output, bsi_output_destroy_notify);
    bsi_output_add_frame_listener(bsi_output, bsi_output_frame_notify);

    bsi_outputs_add(&server->bsi_outputs, bsi_output);
    wlr_output_layout_add_auto(server->wlr_output_layout, wlr_output);
}

int
main(void)
{
    wlr_log_init(WLR_DEBUG, NULL);

    struct bsi_server server;

    server.wl_display = wl_display_create();
    assert(server.wl_display);
    wlr_log(WLR_DEBUG, "created display");

    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);
    wlr_log(WLR_DEBUG, "acquired event loop");

    server.wlr_backend = wlr_backend_autocreate(server.wl_display);
    assert(server.wlr_backend);
    wlr_log(WLR_DEBUG, "autocreate backend done");

    server.wlr_renderer = wlr_renderer_autocreate(server.wlr_backend);
    assert(server.wlr_renderer);
    wlr_log(WLR_DEBUG, "autocreate renderer done");

    wlr_renderer_init_wl_display(server.wlr_renderer, server.wl_display);
    wlr_log(WLR_DEBUG, "initialized wl_display");

    server.wlr_allocator =
        wlr_allocator_autocreate(server.wlr_backend, server.wlr_renderer);
    wlr_log(WLR_DEBUG, "autocreate wlr_allocator done");

    wlr_compositor_create(server.wl_display, server.wlr_renderer);
    wlr_log(WLR_DEBUG, "created wlr_compositor");

    wlr_data_device_manager_create(server.wl_display);
    wlr_log(WLR_DEBUG, "created wlr_data_device_manager");

    server.wlr_output_layout = wlr_output_layout_create();
    wlr_log(WLR_DEBUG, "created output layout");

    struct bsi_outputs bsi_outputs;
    bsi_outputs_init(&bsi_outputs);
    server.bsi_outputs = bsi_outputs;

    struct bsi_listeners bsi_listeners;
    bsi_listeners_init(&bsi_listeners, &server);
    server.bsi_listeners = bsi_listeners;

    bsi_listeners_add_new_output_notify(&server.bsi_listeners,
                                        bsi_listeners_new_output_notify);

    wlr_log(WLR_DEBUG, "attached bsi_output_notify listeners");

    server.wlr_scene = wlr_scene_create();
    wlr_log(WLR_DEBUG, "created wlr_scene");

    wlr_scene_attach_output_layout(server.wlr_scene, server.wlr_output_layout);
    wlr_log(WLR_DEBUG, "attached wlr_output_layout to wlr_scene");

    if (!wlr_backend_start(server.wlr_backend)) {
        wlr_log(WLR_ERROR, "failed to start backend");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    wl_display_run(server.wl_display);

    wl_display_destroy_clients(server.wl_display);
    wl_display_destroy(server.wl_display);

    return 0;
}
