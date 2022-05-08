#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

/**
 * @brief Holds all possible active listener types for `bsi_view`.
 *
 */
enum bsi_view_listener_mask
{
    BSI_VIEW_LISTENER_DESTROY_XDG_SURFACE = 1 << 0,
    BSI_VIEW_LISTENER_PING_TIMEOUT = 1 << 1,
    BSI_VIEW_LISTENER_NEW_POPUP = 1 << 2,
    BSI_VIEW_LISTENER_MAP = 1 << 3,
    BSI_VIEW_LISTENER_UNMAP = 1 << 4,
    BSI_VIEW_LISTENER_CONFIGURE = 1 << 5,
    BSI_VIEW_LISTENER_ACK_CONFIGURE = 1 << 6,
    BSI_VIEW_LISTENER_DESTROY_SCENE_NODE = 1 << 7,
};

#define bsi_view_listener_len 8

/**
 * @brief Represents a single scene node in the compositor scene graph.
 *
 */
struct bsi_view
{
    struct bsi_server* bsi_server;
    struct wlr_xdg_surface* wlr_xdg_surface;
    struct wlr_scene_node* wlr_scene_node;

    // TODO: Add handlers for these events.
    uint32_t active_listeners;
    struct wl_list* active_links[bsi_view_listener_len];
    size_t len_active_links;
    struct
    {
        /* wlr_xdg_surface */
        struct wl_listener destroy_xdg_surface; // TODO
        struct wl_listener ping_timeout;        // TODO
        struct wl_listener new_popup;           // TODO
        struct wl_listener map;                 // TODO
        struct wl_listener unmap;               // TODO
        struct wl_listener configure;           // TODO
        struct wl_listener ack_configure;       // TODO
        /* wlr_scene_node */
        struct wl_listener destroy_scene_node; // TODO
    } events;

    struct wl_list link;
};

void
bsi_view_init(); // TODO

void
bsi_view_listener_unlink_all(); // TODO
