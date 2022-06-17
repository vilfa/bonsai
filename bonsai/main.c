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
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/config/config.h"
#include "bonsai/desktop/view.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

// TODO: Implement xwayland support.
// TODO: Implement input inhibitor - right now, it's faked.
// TODO: Add idle daemon and configuration.
// TODO: Implement server decoration.
// TODO: Investigate weird swipe up/down behavior.
// TODO: Workspaces & multi-output configurations.

int
main(void)
{
#ifdef BSI_DEBUG
    wlr_log_init(WLR_DEBUG, NULL);
#else
    wlr_log_init(WLR_INFO, NULL);
#endif

    struct bsi_config config;
    struct bsi_server server;

    config_init(&config, &server);
    config_parse(&config);

    server_init(&server, &config);
    server_setup(&server);
    server_run(&server);

    wl_display_destroy_clients(server.wl_display);
    wl_display_destroy(server.wl_display);

    wlr_backend_destroy(server.wlr_backend);
    config_destroy(&config);
    server_destroy(&server);

    return EXIT_SUCCESS;
}
