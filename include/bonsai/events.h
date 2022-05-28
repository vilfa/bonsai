#pragma once

#include <wayland-server-core.h>

typedef void
bsi_notify_func_t(struct wl_listener*, void*);

/* bsi_listeners_global */
/* wlr_backend */
extern bsi_notify_func_t bsi_global_backend_new_output_notify;
extern bsi_notify_func_t bsi_global_backend_new_input_notify;
/* wlr_seat */
extern bsi_notify_func_t bsi_global_seat_pointer_grab_begin_notify;
extern bsi_notify_func_t bsi_global_seat_pointer_grab_end_notify;
extern bsi_notify_func_t bsi_global_seat_keyboard_grab_begin_notify;
extern bsi_notify_func_t bsi_global_seat_keyboard_grab_end_notify;
extern bsi_notify_func_t bsi_global_seat_touch_grab_begin_notify; // Skipped.
extern bsi_notify_func_t bsi_global_seat_touch_grab_end_notify;   // Skipped.
extern bsi_notify_func_t bsi_global_seat_request_set_cursor_notify;
extern bsi_notify_func_t bsi_global_seat_request_set_selection_notify;
extern bsi_notify_func_t bsi_global_seat_request_set_primary_selection_notify;
extern bsi_notify_func_t bsi_global_seat_request_start_drag_notify;
/* wlr_xdg_shell */
extern bsi_notify_func_t bsi_global_xdg_shell_new_surface_notify;
/* wlr_layer_shell_v1 */
extern bsi_notify_func_t bsi_layer_shell_new_surface_notify;

// TODO: Get rid of other useless listeners.

/* bsi_view */
extern bsi_notify_func_t bsi_view_destroy_xdg_surface_notify;
extern bsi_notify_func_t bsi_view_map_notify;
extern bsi_notify_func_t bsi_view_unmap_notify;
extern bsi_notify_func_t bsi_view_request_maximize_notify;
extern bsi_notify_func_t bsi_view_request_fullscreen_notify;
extern bsi_notify_func_t bsi_view_request_minimize_notify;
extern bsi_notify_func_t bsi_view_request_move_notify;
extern bsi_notify_func_t bsi_view_request_resize_notify;
extern bsi_notify_func_t bsi_view_request_show_window_menu_notify;

/* bsi_output */
extern bsi_notify_func_t bsi_output_frame_notify;
extern bsi_notify_func_t bsi_output_destroy_notify;

/* bsi_input_{pointer,keyboard} */
extern bsi_notify_func_t bsi_input_pointer_motion_notify;
extern bsi_notify_func_t bsi_input_pointer_motion_absolute_notify;
extern bsi_notify_func_t bsi_input_pointer_button_notify;
extern bsi_notify_func_t bsi_input_pointer_axis_notify;
extern bsi_notify_func_t bsi_input_pointer_frame_notify;
extern bsi_notify_func_t bsi_input_pointer_swipe_begin_notify;
extern bsi_notify_func_t bsi_input_pointer_swipe_update_notify;
extern bsi_notify_func_t bsi_input_pointer_swipe_end_notify;
extern bsi_notify_func_t bsi_input_pointer_pinch_begin_notify;
extern bsi_notify_func_t bsi_input_pointer_pinch_update_notify;
extern bsi_notify_func_t bsi_input_pointer_pinch_end_notify;
extern bsi_notify_func_t bsi_input_pointer_hold_begin_notify;
extern bsi_notify_func_t bsi_input_pointer_hold_end_notify;
extern bsi_notify_func_t bsi_input_keyboard_key_notify;
extern bsi_notify_func_t bsi_input_keyboard_modifiers_notify;
/* bsi_input_{pointer,keyboard} -> wlr_input_device::events::destroy */
extern bsi_notify_func_t bsi_input_device_destroy_notify;

/* bsi_workspace */
extern bsi_notify_func_t bsi_workspace_active_notify;

/* bsi_layer_surface_toplevel */
/* wlr_layer_surface_v1 */
extern bsi_notify_func_t bsi_layer_surface_toplevel_map_notify;
extern bsi_notify_func_t bsi_layer_surface_toplevel_unmap_notify;
extern bsi_notify_func_t bsi_layer_surface_toplevel_destroy_notify;
extern bsi_notify_func_t bsi_layer_surface_toplevel_new_popup_notify;
/* wlr_surface -> wlr_layer_surface::surface */
extern bsi_notify_func_t bsi_layer_surface_toplevel_wlr_surface_commit_notify;
extern bsi_notify_func_t
    bsi_layer_surface_toplevel_wlr_surface_new_subsurface_notify;

/* bsi_layer_surface_popup */
/* wlr_xdg_surface -> wlr_xdg_popup::base */
extern bsi_notify_func_t bsi_layer_surface_popup_destroy_notify;
extern bsi_notify_func_t bsi_layer_surface_popup_new_popup_notify;
extern bsi_notify_func_t bsi_layer_surface_popup_map_notify;
extern bsi_notify_func_t bsi_layer_surface_popup_unmap_notify;

/* bsi_layer_surface_subsurface */
/* wlr_subsurface */
extern bsi_notify_func_t bsi_layer_surface_subsurface_destroy_notify;
extern bsi_notify_func_t bsi_layer_surface_subsurface_map_notify;
extern bsi_notify_func_t bsi_layer_surface_subsurface_unmap_notify;
