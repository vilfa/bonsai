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

#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

// TODO: Take a look at
// https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/examples

int
main(void)
{
    wlr_log_init(WLR_DEBUG, NULL);

    struct bsi_server server;

    bsi_server_init(&server);

    server.wl_socket = wl_display_add_socket_auto(server.wl_display);
    wlr_log(WLR_DEBUG, "Created server socket '%s'", server.wl_socket);

    if (setenv("WAYLAND_DISPLAY", server.wl_socket, true) != 0) {
        wlr_log(WLR_ERROR,
                "Failed to set WAYLAND_DISPLAY env var: %s",
                strerror(errno));
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    if (setenv("WLR_NO_HARDWARE_CURSORS", "1", true) != 0) {
        // Workaround for https://github.com/swaywm/wlroots/issues/3189
        // TODO: Idk man these cursors are weird.
        wlr_log(WLR_ERROR,
                "Failed to set WLR_NO_HARDWARE_CURSORS env var: %s",
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

    wlr_log(WLR_INFO, "Running compositor on socket '%s'", server.wl_socket);
    wl_display_run(server.wl_display);

    wl_display_destroy_clients(server.wl_display);
    wl_display_destroy(server.wl_display);

    return EXIT_SUCCESS;
}
