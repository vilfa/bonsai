#include <assert.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/edges.h>

#include "bonsai/cursor.h"
#include "bonsai/server.h"

struct bsi_cursor*
bsi_cursor_init(struct bsi_cursor* bsi_cursor, struct bsi_server* bsi_server)
{
    assert(bsi_cursor);
    assert(bsi_server);

    bsi_cursor->bsi_server = bsi_server;
    bsi_cursor->wlr_cursor = bsi_server->wlr_cursor;
    bsi_cursor->cursor_mode = BSI_CURSOR_NORMAL;
    bsi_cursor->grab_x = 0.0;
    bsi_cursor->grab_y = 0.0;

    return bsi_cursor;
}

struct bsi_view*
bsi_cursor_view_at(struct bsi_cursor* bsi_cursor,
                   struct wlr_surface** surface_at,
                   double* sx,
                   double* sy)
{
    assert(bsi_cursor);

    struct bsi_server* bsi_server = bsi_cursor->bsi_server;
    /* Search the server root node for the node at the coordinates
     * given by the cursor event. This is not necessarily the topmost node! */
    struct wlr_scene_node* node =
        wlr_scene_node_at(&bsi_server->wlr_scene->node,
                          bsi_server->wlr_cursor->x,
                          bsi_server->wlr_cursor->y,
                          sx,
                          sy);

    /* If this node is not a surface node (e.g. is buffer, rect, root, tree),
     * then return `NULL`. */
    if (node == NULL || node->type != WLR_SCENE_NODE_SURFACE)
        return NULL;

    /* Get the `wlr_surface` of the scene node of the right type. */
    *surface_at = wlr_scene_surface_from_node(node)->surface;

    /* Find the actual topmost node of this node tree. */
    while (node != NULL && node->data == NULL)
        node = node->parent;

    return node->data;
}

void
bsi_cursor_process_motion(struct bsi_cursor* bsi_cursor,
                          union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_cursor);
    assert(bsi_cursor_event.motion);

    if (bsi_cursor->cursor_mode == BSI_CURSOR_MOVE) {
        bsi_cursor_process_view_move(bsi_cursor, bsi_cursor_event);
    } else if (bsi_cursor->cursor_mode == BSI_CURSOR_RESIZE) {
        bsi_cursor_process_view_resize(bsi_cursor, bsi_cursor_event);
    } else {
        /* The cursor is in normal mode, so find the view under the cursor, and
         * send the event to the client that owns it. */
        struct bsi_server* bsi_server = bsi_cursor->bsi_server;
        double sx, sy; /* Surface relative coordinates for our client. */
        struct wlr_surface* surface_at = NULL; /* Surface under cursor. */
        struct bsi_view* bsi_view =
            bsi_cursor_view_at(bsi_cursor, &surface_at, &sx, &sy);

        if (!bsi_view) {
            /* The cursor is in an empty area, set the deafult cursor image.
             * This makes the cursor image appear when moving around the empty
             * desktop. */
            wlr_xcursor_manager_set_cursor_image(
                bsi_server->wlr_xcursor_manager,
                "left_ptr",
                bsi_server->wlr_cursor);
        }

        if (surface_at) {
            /* Notify our surface of the pointer entering and motion. This gives
             * the surface pointer focus, which is different from keyboard
             * focus (e.g. think of scrolling a view in one window, when another
             * window is focused). */
            struct wlr_event_pointer_motion* event = bsi_cursor_event.motion;

            wlr_seat_pointer_notify_enter(
                bsi_server->wlr_seat, surface_at, sx, sy);
            wlr_seat_pointer_notify_motion(
                bsi_server->wlr_seat, event->time_msec, sx, sy);
        } else {
            wlr_seat_pointer_notify_clear_focus(bsi_server->wlr_seat);
        }
    }
}

void
bsi_cursor_process_view_move(struct bsi_cursor* bsi_cursor,
                             union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_cursor);
    assert(bsi_cursor_event.motion);

    struct bsi_server* bsi_server = bsi_cursor->bsi_server;
    struct bsi_view* bsi_view = bsi_cursor->grabbed_view;
    struct wlr_event_pointer_motion* event = bsi_cursor_event.motion;

    bsi_view->x = bsi_server->wlr_cursor->x - bsi_cursor->grab_x;
    bsi_view->y = bsi_server->wlr_cursor->y - bsi_cursor->grab_y;
    wlr_scene_node_set_position(
        bsi_view->wlr_scene_node, bsi_view->x, bsi_view->y);
}

void
bsi_cursor_process_view_resize(struct bsi_cursor* bsi_cursor,
                               union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_cursor);
    assert(bsi_cursor_event.motion);

    // TODO: In a more fleshed-out compositor, you'd wait for the client to
    // prepare a buffer at the new size, then commit any movement that was
    // prepared.

    struct bsi_server* bsi_server = bsi_cursor->bsi_server;
    struct bsi_view* bsi_view = bsi_server->bsi_cursor.grabbed_view;
    struct wlr_event_pointer_motion* event = bsi_cursor_event.motion;

    // TODO: Not sure what is happening here.
    double border_x = bsi_server->wlr_cursor->x - bsi_cursor->grab_x;
    double border_y = bsi_server->wlr_cursor->y - bsi_cursor->grab_y;
    int32_t new_left = bsi_cursor->grab_geobox.x;
    int32_t new_right =
        bsi_cursor->grab_geobox.x + bsi_cursor->grab_geobox.width;
    int32_t new_top = bsi_cursor->grab_geobox.y;
    int32_t new_bottom =
        bsi_cursor->grab_geobox.y + bsi_cursor->grab_geobox.height;

    /* We are constraining the size of the surface to at least 1px width and
     * height? Coordinates start from the top left corner.*/
    if (bsi_cursor->resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;
        if (new_top >= new_bottom)
            new_top = new_bottom - 1;
    } else if (bsi_cursor->resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = border_y;
        if (new_bottom <= new_top)
            new_bottom = new_top + 1;
    }

    if (bsi_cursor->resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;
        if (new_left >= new_right)
            new_left = new_right - 1;
    } else if (bsi_cursor->resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;
        if (new_right <= new_left)
            new_right = new_left + 1;
    }

    struct wlr_box box;
    wlr_xdg_surface_get_geometry(bsi_view->wlr_xdg_surface, &box);
    bsi_view->x = new_left - box.x;
    bsi_view->y = new_top - box.y;
    wlr_scene_node_set_position(
        bsi_view->wlr_scene_node, bsi_view->x, bsi_view->y);

    int32_t new_width = new_right - new_left;
    int32_t new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(bsi_view->wlr_xdg_surface, new_width, new_height);
}
