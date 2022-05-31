#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_server_decoration.h>

#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/input.h"
#include "bonsai/input/cursor.h"
#include "bonsai/output.h"

/**
 * @brief Represents the compositor and its internal state.
 *
 */
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
    struct wlr_server_decoration_manager* wlr_server_decoration_manager;

    /* Global state. */
    bool shutting_down;

    /* Global server listeners. */
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
        /* wlr_server_decoration_manager */
        struct wl_listener new_decoration;
        /* bsi_workspace */
        struct wl_listener workspace_active;
    } listen;

    /* Keeps track of all server outputs. */
    struct
    {
        size_t len;
        struct wl_list outputs;
    } output;

    /* Keeps track of all inputs. */
    struct
    {
        size_t len_pointers;
        struct wl_list pointers;

        size_t len_keyboards;
        struct wl_list keyboards;
    } input;

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
        size_t len_views;
        struct wl_list views;
        size_t len_decorations;
        struct wl_list decorations;
    } scene;

    /* Keeps track of the cursor state. */
    struct
    {
        uint32_t cursor_mode;
        uint32_t cursor_image;
        uint32_t resize_edges;
        struct bsi_view* grabbed_view;
        struct wlr_box grab_box;
        double grab_sx, grab_sy;
    } cursor;
};

struct bsi_server*
bsi_server_init(struct bsi_server* server);

void
bsi_server_listen_init(struct bsi_server* server);

void
bsi_server_output_init(struct bsi_server* server);

void
bsi_server_input_init(struct bsi_server* server);

void
bsi_server_scene_init(struct bsi_server* server);

void
bsi_server_cursor_init(struct bsi_server* server);

void
bsi_server_finish(struct bsi_server* server);

void
bsi_server_exit(struct bsi_server* server);
