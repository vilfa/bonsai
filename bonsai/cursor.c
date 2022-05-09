#include <assert.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>

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

    struct wlr_event_pointer_motion* event = bsi_cursor_event.motion;

#warning "Not implemented"
}

void
bsi_cursor_process_view_resize(struct bsi_cursor* bsi_cursor,
                               union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_cursor);
    assert(bsi_cursor_event.motion);

    struct wlr_event_pointer_motion* event = bsi_cursor_event.motion;

#warning "Not implemented"
}
