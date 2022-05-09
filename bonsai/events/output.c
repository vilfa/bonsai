/**
 * @file output.c
 * @brief Contains all event listeners for `bsi_output`.
 *
 *
 */

#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>

#include "bonsai/config/output.h"
#include "bonsai/events.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

void
bsi_output_destroy_notify(struct wl_listener* listener,
                          __attribute__((unused)) void* data)
{
    struct bsi_output* bsi_output =
        wl_container_of(listener, bsi_output, events.destroy);

    bsi_outputs_remove(&bsi_output->bsi_server->bsi_outputs, bsi_output);
}

void
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
