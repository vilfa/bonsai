/**
 * @file bsi-output.c
 * @brief Contains all event handlers for `bsi_output`.
 *
 *
 */

#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
bsi_output_frame_notify(struct wl_listener* listener,
                        __attribute((unused)) void* data)
{
    bsi_log(WLR_DEBUG, "Got event frame from wlr_output");

    struct bsi_output* output = wl_container_of(listener, output, listen.frame);
    struct wlr_scene* wlr_scene = output->server->wlr_scene;

    struct wlr_scene_output* wlr_scene_output =
        wlr_scene_get_scene_output(wlr_scene, output->wlr_output);

    wlr_scene_output_commit(wlr_scene_output);

    struct timespec now = bsi_util_timespec_get();
    wlr_scene_output_send_frame_done(wlr_scene_output, &now);
}

void
bsi_output_destroy_notify(struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
    bsi_log(WLR_DEBUG, "Got event destroy from wlr_output");

    struct bsi_output* output =
        wl_container_of(listener, output, listen.destroy);
    struct bsi_server* server = output->server;

    bsi_outputs_remove(server, output);
    bsi_output_finish(output);
    bsi_output_destroy(output);

    if (server->output.len == 0) {
        bsi_log(WLR_INFO, "Out of outputs, exiting");
        bsi_server_exit(server);
    }
}
