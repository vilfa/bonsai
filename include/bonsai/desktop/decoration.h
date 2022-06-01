#pragma once

#include <wayland-server-core.h>

#include "bonsai/server.h"

struct bsi_server_decoration
{
    struct bsi_server* server;
    struct wlr_server_decoration* server_decoration;

    struct
    {
        struct wl_listener destroy;
        struct wl_listener mode;
    } listen;

    struct wl_list link_server; // bsi_server
};

struct bsi_server_decoration*
bsi_server_decoration_init(struct bsi_server_decoration* deco,
                           struct bsi_server* server,
                           struct wlr_server_decoration* wlr_server_deco);

void
bsi_server_decoration_destroy(struct bsi_server_decoration* deco);
