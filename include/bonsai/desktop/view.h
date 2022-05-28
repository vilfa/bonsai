#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

#include "bonsai/desktop/workspace.h"
#include "bonsai/input/cursor.h"

/**
 * @brief Represents all surfaces of a single application.
 *
 */
struct bsi_view
{
    struct bsi_server* server;
    struct wlr_xdg_toplevel* toplevel;
    struct wlr_scene_node* scene_node;
    struct bsi_workspace* parent_workspace;

    bool mapped;
    bool maximized, minimized, fullscreen;

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
        struct wl_listener destroy;
        struct wl_listener map;
        struct wl_listener unmap;
        /* wlr_xdg_toplevel */
        struct wl_listener request_maximize;
        struct wl_listener request_fullscreen;
        struct wl_listener request_minimize;
        struct wl_listener request_move;
        struct wl_listener request_resize;
        struct wl_listener request_show_window_menu;
        /* bsi_workspaces */
        struct wl_listener workspace_active;
    } listen;

    struct wl_list link;
    struct wl_list link_workspace;
};

/**
 * @brief Adds a view to the server views.
 *
 * @param bsi_server The server.
 * @param bsi_view The view to add.
 */
void
bsi_scene_add(struct bsi_server* bsi_server, struct bsi_view* bsi_view);

/**
 * @brief Removes a view from the server views.
 *
 * @param bsi_server The server.
 * @param bsi_view The view to remove.
 */
void
bsi_scene_remove(struct bsi_server* bsi_server, struct bsi_view* bsi_view);

/**
 * @brief Initializes a preallocated `bsi_view` representing a scene node.
 *
 * @param bsi_view The view.
 * @param bsi_server The server.
 * @param wlr_xdg_toplevel The xdg toplevel wrapper.
 * @return struct bsi_view* Inititalized `bsi_view`.
 */
struct bsi_view*
bsi_view_init(struct bsi_view* bsi_view,
              struct bsi_server* bsi_server,
              struct wlr_xdg_toplevel* wlr_xdg_toplevel);

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
