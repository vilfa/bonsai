#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include "bonsai/server.h"

struct bsi_decoration
{
    struct bsi_server* server;
    struct bsi_view* view;
    struct wlr_xdg_toplevel_decoration_v1* toplevel_decoration;

    struct
    {
        /* wlr_xdg_toplevel_decoration_v1 */
        struct wl_listener destroy;
        struct wl_listener request_mode;
    } listen;

    struct wl_list link_server; // bsi_server
};

void
bsi_decorations_add(struct bsi_server* server, struct bsi_decoration* deco);

void
bsi_decorations_remove(struct bsi_decoration* deco);

struct bsi_decoration*
bsi_decoration_init(struct bsi_decoration* deco,
                    struct bsi_server* server,
                    struct bsi_view* view,
                    struct wlr_xdg_toplevel_decoration_v1* wlr_deco);

void
bsi_decoration_draw(struct bsi_decoration* deco,
                    struct wlr_texture** texture,
                    float matrix[9],
                    struct wlr_fbox* source_box);

void
bsi_decoration_destroy(struct bsi_decoration* deco);
