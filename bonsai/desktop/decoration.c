#include <stdlib.h>
#include <wayland-util.h>

#include "bonsai/desktop/decoration.h"
#include "bonsai/log.h"

struct bsi_server_decoration*
bsi_server_decoration_init(struct bsi_server_decoration* deco,
                           struct bsi_server* server,
                           struct wlr_server_decoration* wlr_server_deco)
{
    deco->server = server;
    deco->server_decoration = wlr_server_deco;
    return deco;
}

void
bsi_server_decoration_destroy(struct bsi_server_decoration* deco)
{
    wl_list_remove(&deco->listen.destroy.link);
    wl_list_remove(&deco->listen.mode.link);
    free(deco);
}

/**
 * Handlers
 */

void
handle_serverdeco_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_server_decoration");
    struct bsi_server_decoration* server_deco =
        wl_container_of(listener, server_deco, listen.destroy);
    wl_list_remove(&server_deco->link_server);
    bsi_server_decoration_destroy(server_deco);
}

void
handle_serverdeco_mode(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event mode from wlr_server_decoration");
}
