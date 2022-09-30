#pragma once

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/xwayland.h>

#include "bonsai/desktop/decoration.h"
#include "bonsai/desktop/inhibit.h"
#include "bonsai/desktop/view.h"
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

enum bsi_view_type
{
    BSI_VIEW_TYPE_XDG_SHELL,
    BSI_VIEW_TYPE_XWAYLAND,
};

union bsi_xdg_toplevel_event
{
    struct wlr_xdg_toplevel_move_event* move;
    struct wlr_xdg_toplevel_resize_event* resize;
    struct wlr_xdg_toplevel_show_window_menu_event* show_window_menu;
};

struct bsi_view;
struct bsi_view_impl
{
    void (*destroy)(struct bsi_view* view);
    void (*focus)(struct bsi_view* view);
    void (*cursor_interactive)(struct bsi_view* view,
                               enum bsi_cursor_mode cursor_mode,
                               union bsi_xdg_toplevel_event toplevel_event);
    void (*set_maximized)(struct bsi_view* view, bool maximized);
    void (*set_minimized)(struct bsi_view* view, bool minimized);
    void (*set_fullscreen)(struct bsi_view* view, bool fullscreen);
    void (*set_tiled_left)(struct bsi_view* view, bool tiled);
    void (*set_tiled_right)(struct bsi_view* view, bool tiled);
    void (*restore_prev)(struct bsi_view* view);
    bool (*intersects)(struct bsi_view* view, struct wlr_box* box);
    void (*get_correct)(struct bsi_view* view,
                        struct wlr_box* box,
                        struct wlr_box* correction);
    void (*set_correct)(struct bsi_view* view, struct wlr_box* correction);
    void (*request_activate)(struct bsi_view* view);
};

struct bsi_view
{
    enum bsi_view_type type;
    const struct bsi_view_impl* impl;
    struct wlr_scene_tree* tree;

    struct bsi_server* server;
    struct bsi_workspace* workspace;

    bool mapped;
    enum bsi_view_state state;
    enum wlr_xdg_toplevel_decoration_v1_mode decoration_mode;
    struct bsi_xdg_decoration* decoration;

    struct wlr_box geom;

    union
    {
        struct wlr_xdg_toplevel* wlr_xdg_toplevel;
#ifdef BSI_XWAYLAND
        struct wlr_xwayland_surface* wlr_xwayland_surface;
#endif
    };

    struct
    {
        struct bsi_idle_inhibitor* fullscreen;
    } inhibit;

    struct
    {
        struct wl_listener workspace_active;
    } listen;

    struct wl_list link_server;     // bsi_server::scene::views
    struct wl_list link_fullscreen; // bsi_server::scene::views_fullscreen
    struct wl_list link_workspace;  // bsi_workspace::views
};

struct bsi_xdg_shell_view
{
    struct bsi_view view;
    struct
    {
        /* wlr_xdg_surface */
        struct wl_listener map;
        struct wl_listener unmap;
        struct wl_listener destroy;
        /* wlr_xdg_toplevel */
        struct wl_listener request_maximize;
        struct wl_listener request_fullscreen;
        struct wl_listener request_minimize;
        struct wl_listener request_move;
        struct wl_listener request_resize;
        struct wl_listener request_show_window_menu;
    } listen;
};

#ifdef BSI_XWAYLAND
struct bsi_xwayland_view
{
    struct bsi_view view;
    struct
    {
        /* wlr_xwayland_surface */
        struct wl_signal map;
        struct wl_signal unmap;
        struct wl_signal destroy;
        struct wl_signal request_configure;
        struct wl_signal request_move;
        struct wl_signal request_resize;
        struct wl_signal request_minimize;
        struct wl_signal request_maximize;
        struct wl_signal request_fullscreen;
        struct wl_signal request_activate;
    } listen;
};
#endif

struct bsi_view*
view_init(struct bsi_view* view,
          enum bsi_view_type type,
          const struct bsi_view_impl* impl,
          struct bsi_server* server);

void
view_destroy(struct bsi_view* view);

void
view_focus(struct bsi_view* view);

void
view_cursor_interactive(struct bsi_view* view,
                        enum bsi_cursor_mode cursor_mode,
                        union bsi_xdg_toplevel_event toplevel_event);

void
view_set_maximized(struct bsi_view* view, bool maximized);

void
view_set_minimized(struct bsi_view* view, bool minimized);

void
view_set_fullscreen(struct bsi_view* view, bool fullscreen);

void
view_set_tiled_left(struct bsi_view* view, bool tiled);

void
view_set_tiled_right(struct bsi_view* view, bool tiled);

void
view_restore_prev(struct bsi_view* view);

bool
view_intersects(struct bsi_view* view, struct wlr_box* box);

void
view_get_correct(struct bsi_view* view,
                 struct wlr_box* box,
                 struct wlr_box* correction);

void
view_set_correct(struct bsi_view* view, struct wlr_box* correction);

void
view_request_activate(struct bsi_view* view);
