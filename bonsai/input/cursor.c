#include <assert.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/edges.h>

#include "bonsai/desktop/view.h"
#include "bonsai/input/cursor.h"
#include "bonsai/server.h"

void
bsi_cursor_image_set(struct bsi_server* bsi_server,
                     enum bsi_cursor_image bsi_cursor_image)
{
    assert(bsi_server);

    bsi_server->cursor.cursor_image = bsi_cursor_image;
    wlr_xcursor_manager_set_cursor_image(bsi_server->wlr_xcursor_manager,
                                         bsi_cursor_image_map[bsi_cursor_image],
                                         bsi_server->wlr_cursor);
}

struct bsi_view*
bsi_cursor_view_at(struct bsi_server* bsi_server,
                   struct wlr_surface** surface_at,
                   double* sx,
                   double* sy)
{
    assert(bsi_server);

    /* Search the server root node for the node at the coordinates
     * given by the cursor event. This is not necessarily the topmost node! */
    struct wlr_scene_node* node =
        wlr_scene_node_at(&bsi_server->wlr_scene->node,
                          bsi_server->wlr_cursor->x,
                          bsi_server->wlr_cursor->y,
                          sx,
                          sy);

    /* If this node is not a buffer node (eg. is rect, root, tree), then return
     * `NULL`. */
    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
        return NULL;

    struct wlr_scene_buffer* scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface* scene_surface =
        wlr_scene_surface_from_buffer(scene_buffer);

    if (!scene_surface)
        return NULL;

    /* Get the `wlr_surface` of the scene node of the right type. */
    // *surface_at = wlr_scene_surface_from_node(node)->surface;
    *surface_at = scene_surface->surface;

    /* Find the actual topmost node of this node tree. */
    while (node != NULL && node->data == NULL)
        node = node->parent;

    return node->data;
}

void
bsi_cursor_process_motion(struct bsi_server* bsi_server,
                          union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_server);
    assert(bsi_cursor_event.motion);

    if (bsi_server->cursor.cursor_mode == BSI_CURSOR_MOVE) {
        bsi_cursor_process_view_move(bsi_server, bsi_cursor_event);
    } else if (bsi_server->cursor.cursor_mode == BSI_CURSOR_RESIZE) {
        bsi_cursor_process_view_resize(bsi_server, bsi_cursor_event);
    } else {
        /* The cursor is in normal mode, so find the view under the cursor, and
         * send the event to the client that owns it. */
        double sx, sy; /* Surface relative coordinates for our client. */
        struct wlr_surface* surface_at = NULL; /* Surface under cursor. */
        struct bsi_view* bsi_view =
            bsi_cursor_view_at(bsi_server, &surface_at, &sx, &sy);

        if (!bsi_view) {
            /* The cursor is in an empty area, set the deafult cursor image.
             * This makes the cursor image appear when moving around the empty
             * desktop. */
            bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_NORMAL);
        }

        if (surface_at) {
            /* Notify our surface of the pointer entering and motion. This gives
             * the surface pointer focus, which is different from keyboard
             * focus (e.g. think of scrolling a view in one window, when another
             * window is focused). */
            struct wlr_pointer_motion_event* event = bsi_cursor_event.motion;
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
bsi_cursor_process_view_move(struct bsi_server* bsi_server,
                             union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_server);
    assert(bsi_cursor_event.motion);

    struct bsi_view* bsi_view = bsi_server->cursor.grabbed_view;
    struct wlr_pointer_motion_event* event = bsi_cursor_event.motion;

    /* A view in these states cannot be moved. Although, a minimized view
     * getting here is altogether impossible. */
    if (bsi_view->fullscreen || bsi_view->minimized)
        return;

    bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_MOVE);

    struct wlr_box box;
    bsi_view->x = bsi_server->wlr_cursor->x - bsi_server->cursor.grab_x;
    bsi_view->y = bsi_server->wlr_cursor->y - bsi_server->cursor.grab_y;
    wlr_xdg_surface_get_geometry(bsi_view->toplevel->base, &box);
    bsi_view->width = box.width;
    bsi_view->height = box.height;

    if (bsi_view->maximized) {
        bsi_view_set_maximized(bsi_view, false);
        wlr_xdg_toplevel_set_maximized(bsi_view->toplevel, false);
        wlr_xdg_surface_schedule_configure(bsi_view->toplevel->base);
    }

    wlr_scene_node_set_position(bsi_view->scene_node, bsi_view->x, bsi_view->y);
}

void
bsi_cursor_process_view_resize(struct bsi_server* bsi_server,
                               union bsi_cursor_event bsi_cursor_event)
{
    assert(bsi_server);
    assert(bsi_cursor_event.motion);

    // TODO: Make this less shit. Resizing shouldn't be able to move a window.
    // Maybe somewhere between lines 206-212?

    // TODO: In a more fleshed-out compositor, you'd wait for the client to
    // prepare a buffer at the new size, then commit any movement that was
    // prepared.

    struct bsi_view* bsi_view = bsi_server->cursor.grabbed_view;
    struct wlr_pointer_motion_event* event = bsi_cursor_event.motion;

    /* The view cannot be resized when in these states. Although, a minimized
     * view getting here is altogether impossible. */
    if (bsi_view->maximized || bsi_view->minimized || bsi_view->fullscreen)
        return;

    wlr_xdg_toplevel_set_resizing(bsi_view->toplevel, true);
    wlr_xdg_surface_schedule_configure(bsi_view->toplevel->base);

    // TODO: Not sure what is happening here.
    double border_x = bsi_server->wlr_cursor->x - bsi_server->cursor.grab_x;
    double border_y = bsi_server->wlr_cursor->y - bsi_server->cursor.grab_y;
    int32_t new_left = bsi_server->cursor.grab_box.x;
    int32_t new_right =
        bsi_server->cursor.grab_box.x + bsi_server->cursor.grab_box.width;
    int32_t new_top = bsi_server->cursor.grab_box.y;
    int32_t new_bottom =
        bsi_server->cursor.grab_box.y + bsi_server->cursor.grab_box.height;

    /* We are constraining the size of the surface to at least 1px width and
     * height? Coordinates start from the top left corner. */
    if (bsi_server->cursor.resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;
        if (new_top >= new_bottom)
            new_top = new_bottom - 1;
    } else if (bsi_server->cursor.resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = border_y;
        if (new_bottom <= new_top)
            new_bottom = new_top + 1;
    }

    if (bsi_server->cursor.resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;
        if (new_left >= new_right)
            new_left = new_right - 1;
    } else if (bsi_server->cursor.resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;
        if (new_right <= new_left)
            new_right = new_left + 1;
    }

    /* Clients should take care of setting the right cursor. */
    // if (bsi_server->cursor.resize_edges & WLR_EDGE_TOP & WLR_EDGE_LEFT) {
    //     bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_RESIZE_TOP_LEFT);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_TOP &
    //            WLR_EDGE_RIGHT) {
    //     bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_RESIZE_TOP_RIGHT);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_BOTTOM &
    //            WLR_EDGE_LEFT) {
    //     bsi_cursor_image_set(bsi_server,
    //     BSI_CURSOR_IMAGE_RESIZE_BOTTOM_LEFT);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_BOTTOM &
    //            WLR_EDGE_RIGHT) {
    //     bsi_cursor_image_set(bsi_server,
    //     BSI_CURSOR_IMAGE_RESIZE_BOTTOM_RIGHT);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_TOP) {
    //     bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_RESIZE_TOP);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_BOTTOM) {
    //     bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_RESIZE_BOTTOM);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_LEFT) {
    //     bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_RESIZE_LEFT);
    // } else if (bsi_server->cursor.resize_edges & WLR_EDGE_RIGHT) {
    //     bsi_cursor_image_set(bsi_server, BSI_CURSOR_IMAGE_RESIZE_RIGHT);
    // }

    struct wlr_box box;
    wlr_xdg_surface_get_geometry(bsi_view->toplevel->base, &box);
    bsi_view->x = new_left - box.x;
    bsi_view->y = new_top - box.y;
    bsi_view->width = box.width;
    bsi_view->height = box.height;
    wlr_scene_node_set_position(bsi_view->scene_node, bsi_view->x, bsi_view->y);

    int32_t new_width = new_right - new_left;
    int32_t new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(bsi_view->toplevel, new_width, new_height);

    wlr_xdg_toplevel_set_resizing(bsi_view->toplevel, false);
    wlr_xdg_surface_schedule_configure(bsi_view->toplevel->base);
}
