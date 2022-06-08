#define _POSIX_C_SOURCE 200809L
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
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/config/def.h"
#include "bonsai/config/parse.h"
#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

// TODO: Implement working screenshare, look into xdg-desktop-portal.

// TODO: Maybe add multipurpose client for indicating things to the user (e.g.
// took screenshot, switched to workspace).

// TODO: Look into waybar and sway/workspace module implementation, and how to
// make it work.

// TODO: Implement server decoration.

// TODO: Dynamic output resolution setting.

// TODO: Config file.

int
main(void)
{
    wlr_log_init(WLR_DEBUG, NULL);

    struct bsi_server server;
    struct bsi_config config;

    bsi_config_init(&config, &server);
    bsi_config_parse(&config);
    bsi_server_init(&server, &config);

    server.wl_socket = wl_display_add_socket_auto(server.wl_display);
    bsi_debug("Created server socket '%s'", server.wl_socket);

    if (setenv("WAYLAND_DISPLAY", server.wl_socket, true) != 0) {
        bsi_errno("Failed to set WAYLAND_DISPLAY env var");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    if (setenv("WLR_NO_HARDWARE_CURSORS", "1", true) != 0) {
        /* Workaround for https://github.com/swaywm/wlroots/issues/3189
         * Note: Make sure the default cursor theme is set correctly in
         * `/usr/share/icons/default/index.theme` or
         * `$XDG_CONFIG_HOME/.icons/default/index.theme` */
        bsi_errno("Failed to set WLR_NO_HARDWARE_CURSORS env var");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    if (setenv("GDK_BACKEND", "wayland", true) != 0) {
        /* If this is not set, waybar thinks it's running under X. */
        bsi_errno("Failed to set GDK_BACKEND env var");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    if (!wlr_backend_start(server.wlr_backend)) {
        bsi_error("Failed to start backend");
        wlr_backend_destroy(server.wlr_backend);
        wl_display_destroy(server.wl_display);
        exit(EXIT_FAILURE);
    }

    bsi_info("Running compositor on socket '%s'", server.wl_socket);
    wl_display_run(server.wl_display);

    wl_display_destroy_clients(server.wl_display);
    wl_display_destroy(server.wl_display);

    return EXIT_SUCCESS;
}
