#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
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
decorations_add(struct bsi_server* server,
                struct bsi_xdg_decoration* decoration);

void
decorations_remove(struct bsi_xdg_decoration* decoration);

struct bsi_xdg_decoration*
decoration_init(struct bsi_xdg_decoration* decoration,
                struct bsi_server* server,
                struct bsi_view* view,
                struct wlr_xdg_toplevel_decoration_v1* xdg_deco);

void
decoration_update(struct bsi_xdg_decoration* decoration);

void
decoration_iter(struct wlr_scene_buffer* buffer,
                int sx,
                int sy,
                void* user_data);

void
decoration_destroy(struct bsi_xdg_decoration* decoration);
