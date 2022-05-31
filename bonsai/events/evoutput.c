/**
 * @file bsi-output.c
 * @brief Contains all event handlers for `bsi_output`.
 *
 *
 */

#include <wayland-server-core.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
handle_output_frame(struct wl_listener* listener, void* data)
{
    struct bsi_output* output = wl_container_of(listener, output, listen.frame);
    struct wlr_scene* wlr_scene = output->server->wlr_scene;

    struct wlr_scene_output* wlr_scene_output =
        wlr_scene_get_scene_output(wlr_scene, output->output);
    wlr_scene_output_commit(wlr_scene_output);

    struct timespec now = bsi_util_timespec_get();
    wlr_scene_output_send_frame_done(wlr_scene_output, &now);
}

void
handle_output_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_output");

    struct bsi_output* output =
        wl_container_of(listener, output, listen.destroy);
    struct bsi_server* server = output->server;

    if (server->output.len == 1) {
        bsi_info("Last output destroyed, shutting down");
        server->shutting_down = true;
    }

    wlr_output_layout_remove(server->wlr_output_layout, output->output);
    bsi_outputs_remove(server, output);
    bsi_output_destroy(output);

    if (server->output.len == 0) {
        bsi_debug("Out of outputs, exiting");
        bsi_server_exit(server);
    }
}
