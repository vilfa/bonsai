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
#include "bonsai/events.h"
#include "bonsai/scene/cursor.h"
#include "bonsai/scene/view.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

// #define GIMME_ALL_POINTER_EVENTS
#define GIMME_ALL_KEYBOARD_EVENTS

int
main(void)
{
    wlr_log_init(WLR_DEBUG, NULL);

    struct bsi_server server;

    bsi_server_init(&server);

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
