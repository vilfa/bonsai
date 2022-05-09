#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

/**
 * @brief Holds all the views the server owns.
 *
 */
struct bsi_views
{
    size_t len;
    struct wl_list views;
};

/**
 * @brief Holds all possible active listener types for `bsi_view`.
 *
 */
enum bsi_view_listener_mask
{
    /* wlr_xdg_surface */
    BSI_VIEW_LISTENER_DESTROY_XDG_SURFACE = 1 << 0,
    BSI_VIEW_LISTENER_PING_TIMEOUT = 1 << 1,
    BSI_VIEW_LISTENER_NEW_POPUP = 1 << 2,
    BSI_VIEW_LISTENER_MAP = 1 << 3,
    BSI_VIEW_LISTENER_UNMAP = 1 << 4,
    BSI_VIEW_LISTENER_CONFIGURE = 1 << 5,
    BSI_VIEW_LISTENER_ACK_CONFIGURE = 1 << 6,
    /* wlr_xdg_toplevel */
    BSI_VIEW_LISTENER_REQUEST_MAXIMIZE = 1 << 7,
    BSI_VIEW_LISTENER_REQUEST_FULLSCREEN = 1 << 8,
    BSI_VIEW_LISTENER_REQUEST_MINIMIZE = 1 << 9,
    BSI_VIEW_LISTENER_REQUEST_MOVE = 1 << 10,
    BSI_VIEW_LISTENER_REQUEST_RESIZE = 1 << 11,
    BSI_VIEW_LISTENER_REQUEST_SHOW_WINDOW_MENU = 1 << 12,
    BSI_VIEW_LISTENER_SET_PARENT = 1 << 13,
    BSI_VIEW_LISTENER_SET_TITLE = 1 << 14,
    BSI_VIEW_LISTENER_SET_APP_ID = 1 << 15,
    /* wlr_scene_node */
    BSI_VIEW_LISTENER_DESTROY_SCENE_NODE = 1 << 16,
};

#define bsi_view_listener_len 17

/**
 * @brief Represents all surfaces of a single application.
 *
 */
struct bsi_view
{
    struct bsi_server* bsi_server;
    struct wlr_xdg_surface* wlr_xdg_surface;
    struct wlr_scene_node* wlr_scene_node;

    uint32_t active_listeners;
    struct wl_list* active_links[bsi_view_listener_len];
    size_t len_active_links;
    struct
    {
        /* wlr_xdg_surface */
        struct wl_listener destroy_xdg_surface;
        struct wl_listener ping_timeout;
        struct wl_listener new_popup;
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener configure;
        struct wl_listener ack_configure;
        /* wlr_xdg_toplevel */
        struct wl_listener request_maximize;
        struct wl_listener request_fullscreen;
        struct wl_listener request_minimize;
        struct wl_listener request_move;
        struct wl_listener request_resize;
        struct wl_listener request_show_window_menu;
        struct wl_listener set_parent;
        struct wl_listener set_title;
        struct wl_listener set_app_id;
        /* wlr_scene_node */
        struct wl_listener destroy_scene_node;
    } events;

    struct wl_list link;
};

/**
 * @brief Initializes a preallocated `bsi_views`.
 *
 * @param bsi_views The views.
 * @return struct bsi_views* The initialized views.
 */
struct bsi_views*
bsi_views_init(struct bsi_views* bsi_views);

/**
 * @brief Adds a view to the server views.
 *
 * @param bsi_views The views.
 * @param bsi_view The view to add.
 */
void
bsi_views_add(struct bsi_views* bsi_views, struct bsi_view* bsi_view);

/**
 * @brief Removes a view from the server views.
 *
 * @param bsi_views The views.
 * @param bsi_view The view to remove.
 */
void
bsi_views_remove(struct bsi_views* bsi_views, struct bsi_view* bsi_view);

/**
 * @brief Calls `free` for the passed view. Take care to remove the view by
 * calling `bsi_views_remove` before calling this.
 *
 * @param bsi_views The views.
 * @param bsi_view The view to free.
 */
void
bsi_views_free(struct bsi_views* bsi_views, struct bsi_view* bsi_view);

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
 * @brief Focuses the view in the server scene graph.
 *
 * @param bsi_view The view.
 */
void
bsi_view_focus(struct bsi_view* bsi_view);

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
