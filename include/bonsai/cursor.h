#pragma once

#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>

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
 * @brief Holds a single pointer event passed to various functions.
 *
 */
union bsi_cursor_event
{
    struct wlr_event_pointer_motion* motion;
    struct wlr_event_pointer_motion_absolute* motion_absolute;
    struct wlr_event_pointer_button* button;
    struct wlr_event_pointer_axis* axis;
    struct wlr_event_pointer_swipe_begin* swipe_begin;
    struct wlr_event_pointer_swipe_update* swipe_update;
    struct wlr_event_pointer_swipe_end* swipe_end;
    struct wlr_event_pointer_pinch_begin* pinch_begin;
    struct wlr_event_pointer_pinch_update* pinch_update;
    struct wlr_event_pointer_pinch_end* pinch_end;
    struct wlr_event_pointer_hold_begin* hold_begin;
    struct wlr_event_pointer_hold_end* hold_end;
};

/**
 * @brief Holds the global cursor state of the server.
 *
 */
struct bsi_cursor
{
    struct bsi_server* bsi_server;
    struct wlr_cursor* wlr_cursor;

    uint32_t cursor_mode;
    struct bsi_view* grabbed_view;
    struct wlr_box grab_geobox;
    double grab_x, grab_y;
    uint32_t resize_edges;
};

/**
 * @brief Initializes the server cursor state with the provided `wlr_cursor`.
 *
 * @param bsi_cursor The preallocated cursor state.
 * @param bsi_server The server.
 * @return struct bsi_cursor* The initialized cursor state.
 */
struct bsi_cursor*
bsi_cursor_init(struct bsi_cursor* bsi_cursor, struct bsi_server* bsi_server);

/**
 * @brief Returns the `bsi_view` at the cursor event surface coordinates. Pass a
 * pointer to a preallocated `wlr_surface*` variable to get the specific
 * wlr_surface, that is under the cursor and is a member of the view.
 *
 * @param bsi_cursor The cursor state.
 * @param surface_at Pointer to `wlr_surface*` to set to the surface under the
 * cursor.
 * @param sx Pointer to `double` to set to the surface local coordinate x of the
 * surface under the cursor.
 * @param sy Pointer to `double` to set to the surface local coordinate y of the
 * surface under the cursor.
 * @return struct bsi_view* The view under the cursor.
 */
struct bsi_view*
bsi_cursor_view_at(struct bsi_cursor* bsi_cursor,
                   struct wlr_surface** surface_at,
                   double* sx,
                   double* sy);

/**
 * @brief Process the cursor motion.
 *
 * @param bsi_cursor The cursor state.
 * @param time_msec Cursor event being processed.
 */
void
bsi_cursor_process_motion(struct bsi_cursor* bsi_cursor,
                          union bsi_cursor_event bsi_cursor_event);

/**
 * @brief Process view movement with cursor.
 *
 * @param bsi_cursor The cursor state.
 * @param time_msec Cursor event being processed..
 */
void
bsi_cursor_process_view_move(struct bsi_cursor* bsi_cursor,
                             union bsi_cursor_event bsi_cursor_event);

/**
 * @brief
 *
 * @param bsi_cursor The cursor state.
 * @param time_msec Cursor event being processed.
 */
void
bsi_cursor_process_view_resize(struct bsi_cursor* bsi_cursor,
                               union bsi_cursor_event bsi_cursor_event);
