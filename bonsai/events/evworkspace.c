/**
 * @file bsi-workspace.c
 * @brief Contains all event handlers for `bsi_workspace`.
 *
 *
 */
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/events.h"
#include "bonsai/log.h"
#include "bonsai/server.h"

void
bsi_server_workspace_active_notify(
    __attribute__((unused)) struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event active for bsi_server from bsi_workspace");

    struct bsi_workspace* wspace = data;
    wspace->server->active_workspace = wspace;

    if (wspace->output->new) {
        // TODO: Setup external programs for this workspace -- wallpaper, etc.
        bsi_output_setup_extern_progs(wspace->output);
        wspace->output->new = false;
    }

    bsi_debug("Active server workspace is now %ld/%s",
              bsi_workspace_get_global_id(wspace),
              wspace->name);
}

void
bsi_output_workspace_active_notify(
    __attribute__((unused)) struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event active for bsi_output from bsi_workspace");

    struct bsi_workspace* wspace = data;
}

void
bsi_view_workspace_active_notify(
    __attribute__((unused)) struct wl_listener* listener,
    void* data)
{
    bsi_debug("Got event active for bsi_view from bsi_workspace");

    struct bsi_workspace* wspace = data;
}
