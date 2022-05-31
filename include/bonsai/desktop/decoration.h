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

    struct wl_list link;
};

/**
 * @brief Adds a server decoration to the server scene.
 *
 * @param server The server.
 * @param deco The decoration to add.
 */
void
bsi_scene_add_decoration(struct bsi_server* server,
                         struct bsi_server_decoration* deco);

/**
 * @brief Removes a server decoration from the server scene.
 *
 * @param server The server.
 * @param deco The decoration to remove.
 */
void
bsi_scene_remove_decoration(struct bsi_server* server,
                            struct bsi_server_decoration* deco);

struct bsi_server_decoration*
bsi_server_decoration_init(struct bsi_server_decoration* deco,
                           struct bsi_server* server,
                           struct wlr_server_decoration* wlr_server_deco);

void
bsi_server_decoration_finish(struct bsi_server_decoration* deco);

void
bsi_server_decoration_destroy(struct bsi_server_decoration* deco);
