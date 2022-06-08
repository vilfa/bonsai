#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include "bonsai/config/def.h"
#include "bonsai/desktop/lock.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/output.h"

#define bsi_server_extern_prog_len 2
struct bsi_server
{
    /* Globals */
    const char* wl_socket;
    struct wl_display* wl_display;
    struct wlr_backend* wlr_backend;
    struct wlr_renderer* wlr_renderer;
    struct wlr_allocator* wlr_allocator;
    struct wlr_output_layout* wlr_output_layout;
    struct wlr_output_manager_v1* wlr_output_manager;
    struct wlr_scene* wlr_scene;
    struct wlr_xdg_shell* wlr_xdg_shell;
    struct wlr_seat* wlr_seat;
    struct wlr_cursor* wlr_cursor;
    struct wlr_xcursor_manager* wlr_xcursor_manager;
    struct wlr_layer_shell_v1* wlr_layer_shell;
    struct wlr_xdg_decoration_manager_v1* wlr_xdg_decoration_manager;
    struct wlr_xdg_activation_v1* wlr_xdg_activation;
    struct wlr_idle* wlr_idle;
    struct wlr_idle_inhibit_manager_v1* wlr_idle_inhibit_manager;
    struct wlr_session_lock_manager_v1* wlr_session_lock_manager;

    /*
     * Global state
     */
    struct
    {
        /* wlr_backend */
        struct wl_listener new_output;
        struct wl_listener new_input;
        /* wlr_output_layout */
        struct wl_listener output_layout_change;
        /* wlr_output_manager_v1 */
        struct wl_listener output_manager_apply;
        struct wl_listener output_manager_test;
        /* wlr_seat */
        struct wl_listener pointer_grab_begin;
        struct wl_listener pointer_grab_end;
        struct wl_listener keyboard_grab_begin;
        struct wl_listener keyboard_grab_end;
        struct wl_listener touch_grab_begin;
        struct wl_listener touch_grab_end;
        struct wl_listener request_set_cursor;
        struct wl_listener request_set_selection;
        struct wl_listener request_set_primary_selection;
        struct wl_listener request_start_drag;
        /* wlr_xdg_shell */
        struct wl_listener xdg_new_surface;
        /* wlr_layer_shell_v1 */
        struct wl_listener layer_new_surface;
        /* wlr_xdg_decoration_manager_v1 */
        struct wl_listener new_decoration;
        /* wlr_xdg_activation_v1 */
        struct wl_listener request_activate;
        /* wlr_idle_inhibit_manager_v1 */
        struct wl_listener new_inhibitor;
        /* wlr_idle */
        struct wl_listener activity_notify;
        /* wlr_session_lock_manager_v1 */
        struct wl_listener new_lock;
        /* bsi_workspace */
        struct wl_list workspace; // bsi_workspace_listener::link
    } listen;

    struct
    {
        bool extern_setup[bsi_server_extern_prog_len];
        struct wl_list outputs;
    } output;

    struct
    {
        struct wl_list inputs;
    } input;

    struct
    {
        struct wl_list inhibitors;
    } idle;

    struct
    {
        bool locked;
        bool shutting_down;
        struct bsi_session_lock* lock;
    } session;

    struct
    {
        struct bsi_config* all;
        char* wallpaper;
        size_t workspaces_max;
    } config;

    /* So, the way I imagine it, a workspace can be attached to a single output
     * at one time, with each output being able to hold multiple workspaces.
     * ---
     * Each output will also have the four layers defined by
     * zwlr_layer_shell_v1, so the layers are not dependent on the workspace,
     * but on the output. */

    /* Keeps track of the scene and all views. */
    struct bsi_workspace* active_workspace;
    struct
    {
        struct wl_list views;
        struct wl_list views_fullscreen;
        struct wl_list xdg_decorations;
    } scene;

    struct
    {
        uint32_t cursor_mode;
        uint32_t cursor_image;
        uint32_t resize_edges;
        uint32_t swipe_fingers;
        uint32_t swipe_timest; // TODO: Maybe use this for animation.
        bool swipe_cancelled;
        struct bsi_view* grabbed_view;
        struct wlr_box grab_box;
        double grab_sx, grab_sy;
        double swipe_dx, swipe_dy;
    } cursor;
};

enum bsi_server_extern_prog
{
    BSI_SERVER_EXTERN_PROG_WALLPAPER,
    BSI_SERVER_EXTERN_PROG_BAR,
    BSI_SERVER_EXTERN_PROG_MAX,
};

struct bsi_server*
bsi_server_init(struct bsi_server* server, struct bsi_config* config);

void
bsi_server_setup_extern(struct bsi_server* server);

void
bsi_server_finish(struct bsi_server* server);

#undef bsi_server_extern_prog_len
