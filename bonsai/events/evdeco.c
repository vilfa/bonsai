#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/decoration.h"
#include "bonsai/log.h"

void
handle_serverdeco_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_server_decoration");
    struct bsi_server_decoration* server_deco =
        wl_container_of(listener, server_deco, listen.destroy);
    bsi_scene_remove_decoration(server_deco->server, server_deco);
    bsi_server_decoration_destroy(server_deco);
}

void
handle_serverdeco_mode(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event mode from wlr_server_decoration");
}
