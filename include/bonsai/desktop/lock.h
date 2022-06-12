#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/server.h"

struct bsi_session_lock
{
    struct bsi_server* server;
    struct wlr_session_lock_v1* lock;
    struct wlr_scene_tree* tree;
    struct wl_list surfaces; // struct bsi_session_lock_surface

    struct
    {
        struct wl_listener new_surface;
        struct wl_listener unlock;
        struct wl_listener destroy;
    } listen;

    struct wl_list link_server; // bsi_server::session::locks
};

struct bsi_session_lock_surface
{
    struct bsi_session_lock* lock;
    struct wlr_session_lock_surface_v1* lock_surface;
    struct wlr_surface* surface;

    struct
    {
        /* wlr_session_lock_surface_v1 */
        struct wl_listener map;
        struct wl_listener destroy;
        /* wlr_surface */
        struct wl_listener surface_commit;
        /* wlr_output */
        struct wl_listener mode;
        struct wl_listener output_commit;
    } listen;

    struct wl_list link_session_lock; // bsi_session_lock::surfaces
};

struct bsi_session_lock*
session_lock_init(struct bsi_session_lock* lock,
                  struct bsi_server* server,
                  struct wlr_session_lock_v1* wlr_lock);

void
session_lock_destroy(struct bsi_session_lock* lock);

struct bsi_session_lock_surface*
session_lock_surface_init(struct bsi_session_lock_surface* surface,
                          struct bsi_session_lock* lock,
                          struct wlr_session_lock_surface_v1* wlr_lock_surface);

void
session_lock_surface_destroy(struct bsi_session_lock_surface* surface);
