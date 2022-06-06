#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include "bonsai/server.h"

struct bsi_xdg_decoration
{
    struct bsi_server* server;
    struct bsi_view* view;
    struct wlr_xdg_toplevel_decoration_v1* xdg_decoration;

    struct wlr_scene_rect* scene_rect;

    struct
    {
        /* wlr_xdg_toplevel_decoration_v1 */
        struct wl_listener destroy;
        struct wl_listener request_mode;
    } listen;

    struct wl_list link_server; // bsi_server
};

void
bsi_decorations_add(struct bsi_server* server,
                    struct bsi_xdg_decoration* decoration);

void
bsi_decorations_remove(struct bsi_xdg_decoration* decoration);

struct bsi_xdg_decoration*
bsi_decoration_init(struct bsi_xdg_decoration* decoration,
                    struct bsi_server* server,
                    struct bsi_view* view,
                    struct wlr_xdg_toplevel_decoration_v1* xdg_deco);

void
bsi_decoration_update(struct bsi_xdg_decoration* decoration);

void
bsi_decoration_iter(struct wlr_scene_buffer* buffer,
                    int sx,
                    int sy,
                    void* user_data);

void
bsi_decoration_destroy(struct bsi_xdg_decoration* deco);
