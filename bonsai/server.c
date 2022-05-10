#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>

#include "bonsai/server.h"

struct bsi_server*
bsi_server_init(struct bsi_server* bsi_server)
{
    assert(bsi_server);

#warning "Not implemented"
}

void
bsi_server_exit(struct bsi_server* bsi_server)
{
    assert(bsi_server);

    wl_display_terminate(bsi_server->wl_display);
}
