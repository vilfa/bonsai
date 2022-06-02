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
extern bsi_notify_func_t handle_touch_grab_begin_notify;
extern bsi_notify_func_t handle_touch_grab_end_notify;
extern bsi_notify_func_t handle_request_set_cursor_notify;
extern bsi_notify_func_t handle_request_set_selection_notify;
extern bsi_notify_func_t handle_request_set_primary_selection_notify;
extern bsi_notify_func_t handle_request_start_drag_notify;
/* wlr_xdg_shell */
extern bsi_notify_func_t handle_xdgshell_new_surface;
/* wlr_layer_shell_v1 */
extern bsi_notify_func_t handle_layershell_new_surface;
/* wlr_decoration_manager */
extern bsi_notify_func_t handle_deco_manager_new_decoration;

/*
 * bsi_view
 */
extern bsi_notify_func_t handle_xdg_surf_destroy;
extern bsi_notify_func_t handle_xdg_surf_map;
extern bsi_notify_func_t handle_xdg_surf_unmap;
extern bsi_notify_func_t handle_toplvl_request_maximize;
extern bsi_notify_func_t handle_toplvl_request_fullscreen;
extern bsi_notify_func_t handle_toplvl_request_minimize;
extern bsi_notify_func_t handle_toplvl_request_move;
extern bsi_notify_func_t handle_toplvl_request_resize;
extern bsi_notify_func_t handle_toplvl_request_show_window_menu;

/*
 * bsi_output
 */
extern bsi_notify_func_t handle_output_frame;
extern bsi_notify_func_t handle_output_destroy;
extern bsi_notify_func_t handle_output_damage_frame;

/*
 * bsi_input_{pointer,keyboard}
 */
extern bsi_notify_func_t handle_pointer_motion;
extern bsi_notify_func_t handle_pointer_motion_absolute;
extern bsi_notify_func_t handle_pointer_button;
extern bsi_notify_func_t handle_pointer_axis;
extern bsi_notify_func_t handle_pointer_frame;
extern bsi_notify_func_t handle_pointer_swipe_begin;
extern bsi_notify_func_t handle_pointer_swipe_update;
extern bsi_notify_func_t handle_pointer_swipe_end;
extern bsi_notify_func_t handle_pointer_pinch_begin;
extern bsi_notify_func_t handle_pointer_pinch_update;
extern bsi_notify_func_t handle_pointer_pinch_end;
extern bsi_notify_func_t handle_pointer_hold_begin;
extern bsi_notify_func_t handle_pointer_hold_end;
extern bsi_notify_func_t handle_keyboard_key;
extern bsi_notify_func_t handle_keyboard_modifiers;
/* bsi_input_{pointer,keyboard} -> wlr_input_device::events::destroy */
extern bsi_notify_func_t handle_input_device_destroy;

/*
 * bsi_workspace
 */
extern bsi_notify_func_t handle_server_workspace_active;
extern bsi_notify_func_t handle_output_workspace_active;
extern bsi_notify_func_t handle_view_workspace_active;

/*
 * bsi_layer_surface_toplevel
 */
/* wlr_layer_surface_v1 */
extern bsi_notify_func_t handle_layershell_toplvl_map;
extern bsi_notify_func_t handle_layershell_toplvl_unmap;
extern bsi_notify_func_t handle_layershell_toplvl_destroy;
extern bsi_notify_func_t handle_layershell_toplvl_new_popup;
/* wlr_surface -> wlr_layer_surface::surface */
extern bsi_notify_func_t handle_layershell_toplvl_commit;
extern bsi_notify_func_t handle_layershell_toplvl_new_subsurface;

/*
 * bsi_layer_surface_popup
 */
/* wlr_xdg_surface -> wlr_xdg_popup::base */
extern bsi_notify_func_t handle_layershell_popup_map;
extern bsi_notify_func_t handle_layershell_popup_unmap;
extern bsi_notify_func_t handle_layershell_popup_destroy;
extern bsi_notify_func_t handle_layershell_popup_commit;
extern bsi_notify_func_t handle_layershell_popup_new_popup;

/*
 * bsi_layer_surface_subsurface
 */
/* wlr_subsurface */
extern bsi_notify_func_t handle_layershell_subsurface_map;
extern bsi_notify_func_t handle_layershell_subsurface_unmap;
extern bsi_notify_func_t handle_layershell_subsurface_destroy;
extern bsi_notify_func_t handle_layershell_subsurface_commit;

/*
 * bsi_server_decoration
 */
extern bsi_notify_func_t handle_decoration_destroy;
extern bsi_notify_func_t handle_decoration_request_mode;
