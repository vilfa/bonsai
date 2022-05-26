#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/workspace.h"
#include "bonsai/input/cursor.h"

/**
 * @brief Holds all the views the server owns.
 *
 */
struct bsi_views
{
    struct bsi_server* bsi_server;

    size_t len;
    struct wl_list views;
};

/**
 * @brief Represents all surfaces of a single application.
 *
 */
struct bsi_view
{
    struct bsi_server* bsi_server;
    struct wlr_xdg_surface* wlr_xdg_surface;
    struct wlr_scene_node* wlr_scene_node;

    bool mapped;
    bool maximized, minimized, fullscreen;

    char* app_id;
    char* app_title;

    struct bsi_workspace* bsi_workspace; /* Belongs to this workspace. */

    /* Note, that when the window goes fullscreen, minimized or maximized,
     * this will hold the last state of the window that should be restored when
     * restoring the window mode to normal. The helper function
     * bsi_view_restore_prev() does just that. */
    double x, y;
    uint32_t width, height;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
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
        /* bsi_workspaces */
        struct wl_listener active_workspace;
    } listen;

    struct wl_list link;
    struct wl_list link_workspace;
};

/**
 * @brief Initializes a preallocated `bsi_views`.
 *
 * @param bsi_views The views.
 * @return struct bsi_views* The initialized views.
 */
struct bsi_views*
bsi_views_init(struct bsi_views* bsi_views, struct bsi_server* bsi_server);

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
              struct wlr_xdg_surface* wlr_xdg_surface,
              struct bsi_workspace* bsi_workspace);

/**
 * @brief Unlinks all active listeners from a `bsi_view`.
 *
 * @param bsi_view The view.
 */
void
bsi_view_finish(struct bsi_view* bsi_view);

/**
 * @brief Destroys (calls `free`) on the passed view.
 *
 * @param bsi_view The view to destroy.
 */
void
bsi_view_destroy(struct bsi_view* bsi_view);

/**
 * @brief Focuses the view in the server scene graph.
 *
 * @param bsi_view The view.
 */
void
bsi_view_focus(struct bsi_view* bsi_view);

/**
 * @brief Sets the app id for this view.
 *
 * @param bsi_view The view.
 * @param app_id The app id.
 */
void
bsi_view_set_app_id(struct bsi_view* bsi_view, const char* app_id);

/**
 * @brief Sets the app title for this view.
 *
 * @param bsi_view The view.
 * @param app_title The title.
 */
void
bsi_view_set_app_title(struct bsi_view* bsi_view, const char* app_title);

/**
 * @brief Begins interaction with a surface in the view.
 *
 * @param bsi_view The view.
 * @param bsi_cursor_mode The cursor mode.
 * @param edges If this is a resize event, the edges from the resize event.
 */
void
bsi_view_interactive_begin(struct bsi_view* bsi_view,
                           enum bsi_cursor_mode bsi_cursor_mode,
                           uint32_t edges);

void
bsi_view_set_maximized(struct bsi_view* bsi_view, bool maximized);

void
bsi_view_set_minimized(struct bsi_view* bsi_view, bool minimized);

void
bsi_view_set_fullscreen(struct bsi_view* bsi_view, bool fullscreen);

void
bsi_view_restore_prev(struct bsi_view* bsi_view);
