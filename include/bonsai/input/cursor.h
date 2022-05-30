#pragma once

#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>

struct bsi_server;

/**
 * @brief Holds all possible modes for a cursor.
 *
 */
enum bsi_cursor_mode
{
    BSI_CURSOR_NORMAL,
    BSI_CURSOR_MOVE,
    BSI_CURSOR_RESIZE,
};

/**
 * @brief Holds all possible cursor images.
 *
 */
enum bsi_cursor_image
{
    BSI_CURSOR_IMAGE_NORMAL,
    BSI_CURSOR_IMAGE_MOVE,
    BSI_CURSOR_IMAGE_RESIZE_TOP,
    BSI_CURSOR_IMAGE_RESIZE_TOP_LEFT,
    BSI_CURSOR_IMAGE_RESIZE_TOP_RIGHT,
    BSI_CURSOR_IMAGE_RESIZE_BOTTOM,
    BSI_CURSOR_IMAGE_RESIZE_BOTTOM_LEFT,
    BSI_CURSOR_IMAGE_RESIZE_BOTTOM_RIGHT,
    BSI_CURSOR_IMAGE_RESIZE_LEFT,
    BSI_CURSOR_IMAGE_RESIZE_RIGHT,
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static const char* bsi_cursor_image_map[] = {
    [BSI_CURSOR_IMAGE_NORMAL] = "left_ptr",
    [BSI_CURSOR_IMAGE_MOVE] = "move",
    [BSI_CURSOR_IMAGE_RESIZE_TOP] = "top_side",
    [BSI_CURSOR_IMAGE_RESIZE_TOP_LEFT] = "top_left_corner",
    [BSI_CURSOR_IMAGE_RESIZE_TOP_RIGHT] = "top_right_corner",
    [BSI_CURSOR_IMAGE_RESIZE_BOTTOM] = "bottom_side",
    [BSI_CURSOR_IMAGE_RESIZE_BOTTOM_LEFT] = "bottom_left_corner",
    [BSI_CURSOR_IMAGE_RESIZE_BOTTOM_RIGHT] = "bottom_right_corner",
    [BSI_CURSOR_IMAGE_RESIZE_LEFT] = "left_side",
    [BSI_CURSOR_IMAGE_RESIZE_RIGHT] = "right_side",
};
#pragma GCC diagnostic pop

/**
 * @brief Holds a single pointer event passed to various functions.
 *
 */
union bsi_cursor_event
{
    struct wlr_pointer_motion_event* motion;
    struct wlr_pointer_motion_absolute_event* motion_absolute;
    struct wlr_pointer_button_event* button;
    struct wlr_pointer_axis_event* axis;
    struct wlr_pointer_swipe_begin_event* swipe_begin;
    struct wlr_pointer_swipe_update_event* swipe_update;
    struct wlr_pointer_swipe_end_event* swipe_end;
    struct wlr_pointer_pinch_begin_event* pinch_begin;
    struct wlr_pointer_pinch_update_event* pinch_update;
    struct wlr_pointer_pinch_end_event* pinch_end;
    struct wlr_pointer_hold_begin_event* hold_begin;
    struct wlr_pointer_hold_end_event* hold_end;
};

/**
 * @brief Sets the specified cursor image.
 *
 * @param bsi_server The server.
 * @param bsi_cursor_image The image.
 */
void
bsi_cursor_image_set(struct bsi_server* bsi_server,
                     enum bsi_cursor_image bsi_cursor_image);

/**
 * @brief Returns the `bsi_view` at the cursor event surface coordinates. Pass a
 * pointer to a preallocated `wlr_surface*` variable to get the specific
 * wlr_surface, that is under the cursor and is a member of the view.
 *
 * @param bsi_server The server.
 * @param scene_surface_at Pointer to `wlr_scene_surface*` to set to the
 * scene_surface under the cursor.
 * @param surface_at Pointer to `wlr_surface*` to set to the surface under the
 * cursor.
 * @param surface_role The role of the `wlr_surface*`. Check this to know the
 * type of the data pointer returned from this function.
 * @param sx Pointer to `double` to set to the surface local coordinate x of the
 * surface under the cursor.
 * @param sy Pointer to `double` to set to the surface local coordinate y of the
 * surface under the cursor.
 * @return struct bsi_view* The view under the cursor.
 */
void*
bsi_cursor_scene_data_at(struct bsi_server* bsi_server,
                         struct wlr_scene_surface** scene_surface_at,
                         struct wlr_surface** surface_at,
                         const char** surface_role,
                         double* sx,
                         double* sy);

/**
 * @brief Process the cursor motion.
 *
 * @param bsi_server The server.
 * @param time_msec Cursor event being processed.
 */
void
bsi_cursor_process_motion(struct bsi_server* bsi_server,
                          union bsi_cursor_event bsi_cursor_event);

/**
 * @brief Process view movement with cursor.
 *
 * @param bsi_server The server.
 * @param time_msec Cursor event being processed..
 */
void
bsi_cursor_process_view_move(struct bsi_server* bsi_server,
                             union bsi_cursor_event bsi_cursor_event);

/**
 * @brief
 *
 * @param bsi_server The server.
 * @param time_msec Cursor event being processed.
 */
void
bsi_cursor_process_view_resize(struct bsi_server* bsi_server,
                               union bsi_cursor_event bsi_cursor_event);
