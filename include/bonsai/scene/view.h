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

/**
 * @brief Initializes a preallocated `bsi_view` representing a scene node.
 *
 * @param bsi_view The view.
 * @param bsi_server The server.
 * @param wlr_xdg_surface The xdg surface data.
 * @return struct bsi_view* Inititalized `bsi_view`.
 */
struct bsi_view*
bsi_view_init(struct bsi_view* bsi_view,
              struct bsi_server* bsi_server,
              struct wlr_xdg_surface* wlr_xdg_surface);

/**
 * @brief Adds a listener to the scene node represented by `bsi_view`.
 *
 * @param bsi_view The view.
 * @param bsi_listener_type Type of listener to add.
 * @param bsi_listeners_memb Pointer to a listener to initialize with func (a
 * member of one of the `bsi_listeners` anonymus structs).
 * @param bsi_signal_memb Pointer to signal which the listener handles (usually
 * a member of the `events` struct of its parent).
 * @param func The listener func.
 */
void
bsi_view_add_listener(struct bsi_view* bsi_view,
                      enum bsi_view_listener_mask bsi_listener_type,
                      struct wl_listener* bsi_listener_memb,
                      struct wl_signal* bsi_signal_memb,
                      wl_notify_func_t func);

/**
 * @brief Unlinks all active listeners from a `bsi_view`.
 *
 * @param bsi_view The view.
 */
void
bsi_view_listener_unlink_all(struct bsi_view* bsi_view);
