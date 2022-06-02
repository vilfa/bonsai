#include <math.h>
#include <string.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/edges.h>

#include "bonsai/desktop/view.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/input/cursor.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"

void
bsi_cursor_image_set(struct bsi_server* server,
                     enum bsi_cursor_image cursor_image)
{
    server->cursor.cursor_image = cursor_image;
    wlr_xcursor_manager_set_cursor_image(server->wlr_xcursor_manager,
                                         bsi_cursor_image_map[cursor_image],
                                         server->wlr_cursor);
}

void*
bsi_cursor_scene_data_at(struct bsi_server* server,
                         struct wlr_scene_surface** scene_surface_at,
                         struct wlr_surface** surface_at,
                         const char** surface_role,
                         double* sx,
                         double* sy)
{
    /* Search the server root node for the node at the coordinates
     * given by the cursor event. This is not necessarily the topmost node! */
    struct wlr_scene_node* node = wlr_scene_node_at(&server->wlr_scene->node,
                                                    server->wlr_cursor->x,
                                                    server->wlr_cursor->y,
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
    *scene_surface_at = scene_surface;
    *surface_at = scene_surface->surface;
    *surface_role = scene_surface->surface->role->name;

    /* Find the actual topmost node of this node tree. */
    while (node != NULL && node->data == NULL)
        node = node->parent;

    return node->data;
}

void
bsi_cursor_process_motion(struct bsi_server* server,
                          union bsi_cursor_event cursor_event)
{
    if (server->cursor.cursor_mode == BSI_CURSOR_MOVE) {
        bsi_cursor_process_view_move(server, cursor_event);
    } else if (server->cursor.cursor_mode == BSI_CURSOR_RESIZE) {
        bsi_cursor_process_view_resize(server, cursor_event);
    } else {
        /* The cursor is in normal mode, so find the view under the cursor, and
         * send the event to the client that owns it. */
        double sx, sy; /* Surface relative coordinates for our client. */
        struct wlr_scene_surface* scene_surface_at = NULL;
        struct wlr_surface* surface_at = NULL; /* Surface under cursor. */
        const char* surface_role = NULL;

        void* scene_data = bsi_cursor_scene_data_at(
            server, &scene_surface_at, &surface_at, &surface_role, &sx, &sy);

        if (scene_data == NULL) {
            /* The cursor is in an empty area, set the deafult cursor image.
             * This makes the cursor image appear when moving around the empty
             * desktop. */
            bsi_cursor_image_set(server, BSI_CURSOR_IMAGE_NORMAL);
            return;
        }

        if (surface_at) {
            /* Notify our surface of the pointer entering and motion. This gives
             * the surface pointer focus, which is different from keyboard
             * focus (e.g. think of scrolling a view in one window, when another
             * window is focused). */
            struct wlr_pointer_motion_event* e = cursor_event.motion;
            wlr_seat_pointer_notify_enter(server->wlr_seat, surface_at, sx, sy);
            wlr_seat_pointer_notify_motion(
                server->wlr_seat, e->time_msec, sx, sy);
        } else {
            wlr_seat_pointer_notify_clear_focus(server->wlr_seat);
        }
    }
}

void
bsi_cursor_process_view_move(struct bsi_server* server,
                             union bsi_cursor_event cursor_event)
{
    struct bsi_view* view = server->cursor.grabbed_view;
    struct wlr_pointer_motion_event* event = cursor_event.motion;

    /* A view that is minimized or fullscreen cannot be moved. Although, a
     * minimized view getting here is altogether impossible. */
    if (view->state & BSI_VIEW_STATE_MINIMIZED ||
        view->state & BSI_VIEW_STATE_FULLSCREEN)
        return;

    bsi_cursor_image_set(server, BSI_CURSOR_IMAGE_MOVE);

    if (view->state == BSI_VIEW_STATE_MAXIMIZED) {
        bsi_view_set_maximized(view, false);
        view->box.x = server->wlr_cursor->x - (double)view->box.width / 2;
        view->box.y = server->wlr_cursor->y - server->cursor.grab_sy;
        server->cursor.grab_sx = (double)view->box.width / 2;
        server->cursor.grab_sy = 0;
        bsi_debug("Unmaximize by move, surface coords are (%d, %d)",
                  view->box.x,
                  view->box.y);
    } else {
        view->box.x = server->wlr_cursor->x - server->cursor.grab_sx;
        view->box.y = server->wlr_cursor->y - server->cursor.grab_sy;
    }

    bsi_debug("Moving view to coords (%d, %d)", view->box.x, view->box.y);
    wlr_scene_node_set_position(view->scene_node, view->box.x, view->box.y);
}

void
bsi_cursor_process_view_resize(struct bsi_server* server,
                               union bsi_cursor_event cursor_event)
{
    struct bsi_view* view = server->cursor.grabbed_view;
    struct wlr_pointer_motion_event* event = cursor_event.motion;

    /* The view cannot be resized when in these states. Although, a minimized
     * view getting here is altogether impossible. */
    if (view->state != BSI_VIEW_STATE_NORMAL)
        return;

    wlr_xdg_toplevel_set_resizing(view->toplevel, true);

    double rsiz_lx, rsiz_ly;
    int32_t new_left, new_right, new_top, new_bottom;
    /* The layout local coordinates of the cursor resizing. */
    rsiz_lx = server->wlr_cursor->x - server->cursor.grab_sx;
    rsiz_ly = server->wlr_cursor->y - server->cursor.grab_sy;
    /* New positions of view edges. */
    new_left = server->cursor.grab_box.x;
    new_right = server->cursor.grab_box.x + server->cursor.grab_box.width;
    new_top = server->cursor.grab_box.y;
    new_bottom = server->cursor.grab_box.y + server->cursor.grab_box.height;

    /* Constrain the opposite edges to each other. */
    if (server->cursor.resize_edges & WLR_EDGE_TOP) {
        new_top = rsiz_ly;
        if (new_top >= new_bottom)
            new_top = new_bottom - 1;
    } else if (server->cursor.resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = rsiz_ly;
        if (new_bottom <= new_top)
            new_bottom = new_top + 1;
    }
    if (server->cursor.resize_edges & WLR_EDGE_LEFT) {
        new_left = rsiz_lx;
        if (new_left >= new_right)
            new_left = new_right - 1;
    } else if (server->cursor.resize_edges & WLR_EDGE_RIGHT) {
        new_right = rsiz_lx;
        if (new_right <= new_left)
            new_right = new_left + 1;
    }

    struct wlr_box box;
    wlr_xdg_surface_get_geometry(view->toplevel->base, &box);

    /* Set new view position. Account for possible titlebars, etc. */
    view->box.x = new_left - box.x;
    view->box.y = new_top - box.y;
    view->box.width = box.width;
    view->box.height = box.height;
    wlr_scene_node_set_position(view->scene_node, view->box.x, view->box.y);

    /* Set new view size. */
    wlr_xdg_toplevel_set_size(
        view->toplevel, new_right - new_left, new_bottom - new_top);

    wlr_xdg_toplevel_set_resizing(view->toplevel, false);
}

void
bsi_cursor_process_swipe(struct bsi_server* server,
                         union bsi_cursor_event cursor_event)
{
    if (server->cursor.cursor_mode != BSI_CURSOR_SWIPE)
        return;

    struct wlr_pointer_swipe_update_event* event = cursor_event.swipe_update;

    if (fabs(event->dx) > fabs(event->dy)) {
        /* This is a horizontal swipe event. */
        server->cursor.swipe_dx += event->dx;
    } else if (fabs(event->dy) > fabs(event->dx)) {
        /* This is a vertical swipe event. */
        server->cursor.swipe_dy += event->dy;
    } else {
        /* This is very very very unlikely, but hey, it's not impossible. */
        bsi_debug("Whoa, exactly equal swipe directions");
    }
}
