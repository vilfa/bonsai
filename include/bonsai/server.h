#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include "bonsai/config/config.h"
#include "bonsai/desktop/decoration.h"
#include "bonsai/desktop/layers.h"
#include "bonsai/desktop/lock.h"
#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/output.h"

struct bsi_server
{
    const char* wl_socket;
    struct wl_display* wl_display;
    struct wlr_backend* wlr_backend;
    struct wlr_renderer* wlr_renderer;
    struct wlr_allocator* wlr_allocator;
    struct wlr_output_layout* wlr_output_layout;
    struct wlr_scene* wlr_scene;
    struct wlr_xdg_shell* wlr_xdg_shell;
    struct wlr_seat* wlr_seat;
    struct wlr_cursor* wlr_cursor;
    struct wlr_idle* wlr_idle;
    struct wlr_layer_shell_v1* wlr_layer_shell;
    struct wlr_xdg_activation_v1* wlr_xdg_activation;
    struct wlr_output_manager_v1* wlr_output_manager;
    struct wlr_xcursor_manager* wlr_xcursor_manager;
    struct wlr_xdg_decoration_manager_v1* wlr_xdg_decoration_manager;
    struct wlr_idle_inhibit_manager_v1* wlr_idle_inhibit_manager;
    struct wlr_input_inhibit_manager* wlr_input_inhbit_manager;
    struct wlr_session_lock_manager_v1* wlr_session_lock_manager;

    struct bsi_input_manager* bsi_input_manager;

    struct
    {
        /* wlr_backend */
        struct wl_listener new_output;
        // struct wl_listener new_input; /* Moved to bsi_input_manager. */
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
        struct wl_listener request_set_cursor;
        struct wl_listener request_set_selection;
        struct wl_listener request_set_primary_selection;
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
        /* wlr_input_inhibit_manager */
        struct wl_listener input_inhibit_activate;
        struct wl_listener input_inhibit_deactivate;
        /* bsi_workspace */
        struct wl_list workspace; // bsi_workspace_listener::link
    } listen;

    struct
    {
#define len_extern_progs 2
        bool setup[len_extern_progs];
#undef len_extern_progs
        struct wl_list outputs;
    } output;

    struct
    {
        struct wl_list inputs;
    } input;

    struct
    {
        struct wl_list idle;
        struct wl_list input;
    } inhibitors;

    struct
    {
        bool locked;
        bool shutting_down;
        struct bsi_session_lock* lock;
    } session;

    struct
    {
        struct bsi_config* config;
        char* wallpaper;
        size_t workspaces;
        struct wl_list input;
    } config;

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
        uint32_t swipe_timest;
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
server_init(struct bsi_server* server, struct bsi_config* config);

void
server_setup(struct bsi_server* server);

void
server_run(struct bsi_server* server);

void
server_destroy(struct bsi_server* server);

/* Outputs */
void
outputs_add(struct bsi_server* server, struct bsi_output* output);

void
outputs_remove(struct bsi_output* output);

struct bsi_output*
outputs_find(struct bsi_server* server, struct wlr_output* wlr_output);

struct bsi_output*
outputs_find_not(struct bsi_server* server, struct bsi_output* not_output);

void
outputs_setup_extern(struct bsi_server* server);

/* Inputs */
void
inputs_add(struct bsi_server* server, struct bsi_input_device* device);

void
inputs_remove(struct bsi_input_device* device);

/* Workspaces */
void
workspaces_add(struct bsi_output* output, struct bsi_workspace* workspace);

void
workspaces_remove(struct bsi_output* output, struct bsi_workspace* workspace);

struct bsi_workspace*
workspaces_get_active(struct bsi_output* output);

void
workspaces_next(struct bsi_output* output);

void
workspaces_prev(struct bsi_output* output);

/* Views */
void
views_add(struct bsi_server* server, struct bsi_view* view);

void
views_remove(struct bsi_view* view);

void
views_focus_recent(struct bsi_server* server);

struct bsi_view*
views_get_focused(struct bsi_server* server);

/* Layers */
void
layers_add(struct bsi_output* output, struct bsi_layer_surface_toplevel* layer);

void
layers_remove(struct bsi_layer_surface_toplevel* layer);

/* Idle inhibitors */
void
idle_inhibitors_add(struct bsi_server* server,
                    struct bsi_idle_inhibitor* inhibitor);

void
idle_inhibitors_remove(struct bsi_idle_inhibitor* inhibitor);

void
idle_inhibitors_update(struct bsi_server* server);

/* Decorations */
void
decorations_add(struct bsi_server* server,
                struct bsi_xdg_decoration* decoration);

void
decorations_remove(struct bsi_xdg_decoration* decoration);
