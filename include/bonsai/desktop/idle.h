#pragma once

#include <wayland-server-core.h>

enum bsi_idle_inhibit_mode
{
    BSI_IDLE_INHIBIT_APPLICATION, // Application set inhbitor
    BSI_IDLE_INHIBIT_FULLSCREEN,  // User set inhbitor, when app is fullscreen
    BSI_IDLE_INHIBIT_USER,        // Arbitrary user set inhibitor
};

struct bsi_idle_inhibitor
{
    struct bsi_server* server;
    struct wlr_idle_inhibitor_v1* inhibitor;
    struct wlr_idle_inhibit_manager_v1* manager;
    struct bsi_view* view;
    enum bsi_idle_inhibit_mode mode;

    struct
    {
        /* wlr_idle_inhibitor_v1 */
        struct wl_listener destroy;
    } listen;

    struct wl_list link_server; // bsi_server::idle
};

void
idle_inhibitors_add(struct bsi_server* server,
                    struct bsi_idle_inhibitor* inhibitor);

void
idle_inhibitors_remove(struct bsi_idle_inhibitor* inhibitor);

void
idle_inhibitors_update(struct bsi_server* server);

struct bsi_idle_inhibitor*
idle_inhibitor_init(struct bsi_idle_inhibitor* inhibitor,
                    struct wlr_idle_inhibitor_v1* wlr_inhibitor,
                    struct bsi_server* server,
                    struct bsi_view* view,
                    enum bsi_idle_inhibit_mode mode);

void
idle_inhibitor_destroy(struct bsi_idle_inhibitor* inhibitor);

bool
idle_inhibitor_active(struct bsi_idle_inhibitor* inhibitor);
