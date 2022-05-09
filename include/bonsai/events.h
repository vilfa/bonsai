#pragma once

#include <wayland-server-core.h>

typedef void
bsi_notify_func_t(struct wl_listener*, void*);

/* All `bsi_listeners` listeners. */
extern bsi_notify_func_t bsi_listeners_new_output_notify;
extern bsi_notify_func_t bsi_listeners_new_input_notify;
extern bsi_notify_func_t bsi_listeners_destroy_notify;
extern bsi_notify_func_t bsi_listeners_new_xdg_surface_notify;

/* All `bsi_view` listeners. */
extern bsi_notify_func_t bsi_view_destroy_xdg_surface_notify;
extern bsi_notify_func_t bsi_view_destroy_scene_node_notify;
extern bsi_notify_func_t bsi_view_ping_timeout_notify;
extern bsi_notify_func_t bsi_view_new_popup_notify;
extern bsi_notify_func_t bsi_view_map_notify;
extern bsi_notify_func_t bsi_view_unmap_notify;
extern bsi_notify_func_t bsi_view_configure_notify;
extern bsi_notify_func_t bsi_view_ack_configure_notify;
extern bsi_notify_func_t bsi_view_request_maximize_notify;
extern bsi_notify_func_t bsi_view_request_fullscreen_notify;
extern bsi_notify_func_t bsi_view_request_minimize_notify;
extern bsi_notify_func_t bsi_view_request_move_notify;
extern bsi_notify_func_t bsi_view_request_resize_notify;
extern bsi_notify_func_t bsi_view_request_show_window_menu_notify;
extern bsi_notify_func_t bsi_view_set_parent_notify;
extern bsi_notify_func_t bsi_view_set_title_notify;
extern bsi_notify_func_t bsi_view_set_app_id_notify;

/* All `bsi_output` listeners. */
extern bsi_notify_func_t bsi_output_destroy_notify;
extern bsi_notify_func_t bsi_output_frame_notify;
