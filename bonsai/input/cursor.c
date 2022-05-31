#include <assert.h>
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
#include "bonsai/input/cursor.h"
#include "bonsai/log.h"
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
    // *surface_at = wlr_scene_surface_from_node(node)->surface;
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
                          union bsi_cursor_event event)
{
    if (server->cursor.cursor_mode == BSI_CURSOR_MOVE) {
        bsi_cursor_process_view_move(server, event);
    } else if (server->cursor.cursor_mode == BSI_CURSOR_RESIZE) {
        bsi_cursor_process_view_resize(server, event);
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
            struct wlr_pointer_motion_event* e = event.motion;
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
                             union bsi_cursor_event event)
{
    struct bsi_view* view = server->cursor.grabbed_view;
    struct wlr_pointer_motion_event* e = event.motion;

    /* A view in these states cannot be moved. Although, a minimized view
     * getting here is altogether impossible. */
    if (view->fullscreen || view->minimized)
        return;

    bsi_cursor_image_set(server, BSI_CURSOR_IMAGE_MOVE);

    if (view->maximized) {
        bsi_view_set_maximized(view, false);
        view->x = server->wlr_cursor->x - (float)view->width / 2;
        view->y = server->wlr_cursor->y - server->cursor.grab_sy;
        wlr_xdg_toplevel_set_maximized(view->toplevel, false);
        wlr_xdg_surface_schedule_configure(view->toplevel->base);

        // TODO: Scene node position simply does not want to be set.

        bsi_debug("Unmaximize by move, surface coords are (%.2lf, %.2lf)",
                  view->x,
                  view->y);
    } else {
        view->x = server->wlr_cursor->x - server->cursor.grab_sx;
        view->y = server->wlr_cursor->y - server->cursor.grab_sy;
    }

    wlr_scene_node_set_position(view->scene_node, view->x, view->y);
}

void
bsi_cursor_process_view_resize(struct bsi_server* server,
                               union bsi_cursor_event event)
{
    assert(server);
    assert(event.motion);

    // TODO: Make this less shit. Resizing shouldn't be able to move a window.
    // Maybe somewhere between lines 206-212?

    // TODO: In a more fleshed-out compositor, you'd wait for the client to
    // prepare a buffer at the new size, then commit any movement that was
    // prepared.

    struct bsi_view* view = server->cursor.grabbed_view;
    struct wlr_pointer_motion_event* e = event.motion;

    /* The view cannot be resized when in these states. Although, a minimized
     * view getting here is altogether impossible. */
    if (view->maximized || view->minimized || view->fullscreen)
        return;

    wlr_xdg_toplevel_set_resizing(view->toplevel, true);
    wlr_xdg_surface_schedule_configure(view->toplevel->base);

    // TODO: Not sure what is happening here.
    double border_x = server->wlr_cursor->x - server->cursor.grab_sx;
    double border_y = server->wlr_cursor->y - server->cursor.grab_sy;
    int32_t new_left = server->cursor.grab_box.x;
    int32_t new_right =
        server->cursor.grab_box.x + server->cursor.grab_box.width;
    int32_t new_top = server->cursor.grab_box.y;
    int32_t new_bottom =
        server->cursor.grab_box.y + server->cursor.grab_box.height;

    /* We are constraining the size of the surface to at least 1px width and
     * height? Coordinates start from the top left corner. */
    if (server->cursor.resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;
        if (new_top >= new_bottom)
            new_top = new_bottom - 1;
    } else if (server->cursor.resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = border_y;
        if (new_bottom <= new_top)
            new_bottom = new_top + 1;
    }

    if (server->cursor.resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;
        if (new_left >= new_right)
            new_left = new_right - 1;
    } else if (server->cursor.resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;
        if (new_right <= new_left)
            new_right = new_left + 1;
    }

    struct wlr_box box;
    wlr_xdg_surface_get_geometry(view->toplevel->base, &box);
    view->x = new_left - box.x;
    view->y = new_top - box.y;
    view->width = box.width;
    view->height = box.height;
    wlr_scene_node_set_position(view->scene_node, view->x, view->y);

    int32_t new_width = new_right - new_left;
    int32_t new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(view->toplevel, new_width, new_height);

    wlr_xdg_toplevel_set_resizing(view->toplevel, false);
    wlr_xdg_surface_schedule_configure(view->toplevel->base);
}
