#pragma once

#include <wayland-server-core.h>

typedef void
bsi_notify_func_t(struct wl_listener*, void*);

/*
 * bsi_server
 */
/* wlr_backend */
extern bsi_notify_func_t handle_new_output;
extern bsi_notify_func_t handle_new_input;
/* wlr_output_layout */
extern bsi_notify_func_t handle_output_layout_change;
/* wlr_output_manager_v1 */
extern bsi_notify_func_t handle_output_manager_apply;
extern bsi_notify_func_t handle_output_manager_test;
/* wlr_seat */
extern bsi_notify_func_t handle_pointer_grab_begin_notify;
extern bsi_notify_func_t handle_pointer_grab_end_notify;
extern bsi_notify_func_t handle_keyboard_grab_begin_notify;
extern bsi_notify_func_t handle_keyboard_grab_end_notify;
extern bsi_notify_func_t handle_request_set_cursor_notify;
extern bsi_notify_func_t handle_request_set_selection_notify;
extern bsi_notify_func_t handle_request_set_primary_selection_notify;
/* wlr_xdg_shell */
extern bsi_notify_func_t handle_xdg_shell_new_surface;
/* wlr_layer_shell_v1 */
extern bsi_notify_func_t handle_layer_shell_new_surface;
/* wlr_xdg_decoration_manager_v1 */
extern bsi_notify_func_t handle_xdg_decoration_manager_new_decoration;
/* wlr_xdg_activation_v1 */
extern bsi_notify_func_t handle_xdg_request_activate;
/* wlr_idle_inhibit_manager_v1 */
extern bsi_notify_func_t handle_idle_manager_new_inhibitor;
extern bsi_notify_func_t handle_idle_activity_notify;
/* wlr_session_lock_manager_v1 */
extern bsi_notify_func_t handle_session_lock_manager_new_lock;

/*
 * bsi_workspace
 */
extern bsi_notify_func_t handle_server_workspace_active;
extern bsi_notify_func_t handle_output_workspace_active;
extern bsi_notify_func_t handle_view_workspace_active;
