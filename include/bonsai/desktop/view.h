#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "bonsai/desktop/workspace.h"
#include "bonsai/input/cursor.h"

enum bsi_view_state
{
    BSI_VIEW_STATE_NORMAL = 1 << 0,
    BSI_VIEW_STATE_MINIMIZED = 1 << 1,
    BSI_VIEW_STATE_MAXIMIZED = 1 << 2,
    BSI_VIEW_STATE_FULLSCREEN = 1 << 3,
    BSI_VIEW_STATE_TILED_LEFT = 1 << 4,
    BSI_VIEW_STATE_TILED_RIGHT = 1 << 5,
};

struct bsi_view
{
    struct bsi_server* server;
    struct wlr_xdg_toplevel* toplevel;
    struct wlr_scene_node* scene_node;
    struct bsi_workspace* parent_workspace;

    bool mapped;
    enum bsi_view_state state;

    /* Note, that when the window goes fullscreen, minimized or maximized,
     * this will hold the last state of the window that should be restored when
     * restoring the window mode to normal. The helper function
     * bsi_view_restore_prev() does just that. */
    struct wlr_box box;

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

    struct wl_list link_server;    // bsi_server::scene::{views,views_minimized}
    struct wl_list link_workspace; // bsi_workspace::views
    struct wl_list link_recent;    // bsi_server::scene::views_recent
};

union bsi_view_toplevel_event
{
    struct wlr_xdg_toplevel_move_event* move;
    struct wlr_xdg_toplevel_resize_event* resize;
    struct wlr_xdg_toplevel_show_window_menu_event* show_window_menu;
};

/**
 * @brief Adds a view to the server views.
 *
 * @param server The server.
 * @param view The view to add.
 */
void
bsi_views_add(struct bsi_server* server, struct bsi_view* view);

/**
 * @brief Adds a view to the minimized views.
 *
 * @param server The server.
 * @param view The minimized view.
 */
void
bsi_views_add_minimized(struct bsi_server* server, struct bsi_view* view);

/**
 * @brief Focuses the most recently used view, if any exists.
 *
 * @param server The server.
 */
void
bsi_views_mru_focus(struct bsi_server* server);

struct bsi_view*
bsi_views_get_focused(struct bsi_server* server);

/**
 * @brief Removes a view from any of the server views.
 *
 * @param server The server.
 * @param view The view to remove.
 */
void
bsi_views_remove(struct bsi_view* view);

/**
 * @brief Initializes a preallocated `bsi_view` representing a scene node.
 *
 * @param view The view.
 * @param server The server.
 * @param toplevel The xdg toplevel wrapper.
 * @return struct bsi_view* Inititalized `bsi_view`.
 */
struct bsi_view*
bsi_view_init(struct bsi_view* view,
              struct bsi_server* server,
              struct wlr_xdg_toplevel* toplevel);

/**
 * @brief Unlinks all listeners and frees the view.
 *
 * @param view The view to destroy.
 */
void
bsi_view_destroy(struct bsi_view* view);

/**
 * @brief Focuses the view in the server scene graph.
 *
 * @param view The view.
 */
void
bsi_view_focus(struct bsi_view* view);

/**
 * @brief Begins interaction with a surface in the view.
 *
 * @param view The view.
 * @param cursor_mode The cursor mode.
 * @param toplevel_event The xdg toplevel event.
 */
void
bsi_view_interactive_begin(struct bsi_view* view,
                           enum bsi_cursor_mode cursor_mode,
                           union bsi_view_toplevel_event toplevel_event);

void
bsi_view_set_maximized(struct bsi_view* view, bool maximized);

void
bsi_view_set_minimized(struct bsi_view* view, bool minimized);

void
bsi_view_set_fullscreen(struct bsi_view* view, bool fullscreen);

void
bsi_view_set_tiled_left(struct bsi_view* view, bool tiled);

void
bsi_view_set_tiled_right(struct bsi_view* view, bool tiled);

void
bsi_view_restore_prev(struct bsi_view* view);
