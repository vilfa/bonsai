#include <assert.h>
#include <stdbool.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "bonsai/scene/view.h"
#include "bonsai/server.h"

struct bsi_views*
bsi_views_init(struct bsi_views* bsi_views)
{
    assert(bsi_views);

    bsi_views->len = 0;
    wl_list_init(&bsi_views->views);

    return bsi_views;
}

void
bsi_views_add(struct bsi_views* bsi_views, struct bsi_view* bsi_view)
{
    assert(bsi_views);
    assert(bsi_view);

    ++bsi_views->len;
    wl_list_insert(&bsi_views->views, &bsi_view->link);
}

void
bsi_views_remove(struct bsi_views* bsi_views, struct bsi_view* bsi_view)
{
    assert(bsi_views);
    assert(bsi_view);

    --bsi_views->len;
    wl_list_remove(&bsi_view->link);
}

struct bsi_view*
bsi_view_init(struct bsi_view* bsi_view,
              struct bsi_server* bsi_server,
              struct wlr_xdg_surface* wlr_xdg_surface)
{
    assert(bsi_view);
    assert(bsi_server);
    assert(wlr_xdg_surface);

    bsi_view->bsi_server = bsi_server;
    bsi_view->wlr_xdg_surface = wlr_xdg_surface;
    bsi_view->active_listeners = 0;
    bsi_view->len_active_links = 0;
    /* Create a new node from the root server node. */
    bsi_view->wlr_scene_node = wlr_scene_xdg_surface_create(
        &bsi_view->bsi_server->wlr_scene->node, bsi_view->wlr_xdg_surface);
    bsi_view->wlr_scene_node->data = bsi_view;
    wlr_xdg_surface->data = bsi_view->wlr_scene_node;

    return bsi_view;
}

void
bsi_view_destroy(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    free(bsi_view);
}

void
bsi_view_focus(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    struct bsi_server* bsi_server = bsi_view->bsi_server;
    struct bsi_views* bsi_views = &bsi_server->bsi_views;
    struct wlr_seat* wlr_seat = bsi_server->wlr_seat;
    struct wlr_surface* prev_focused = wlr_seat->keyboard_state.focused_surface;

    if (prev_focused == bsi_view->wlr_xdg_surface->surface)
        return;

    if (prev_focused) {
        /* Deactivate the previously focused surface and notify the client. */
        struct wlr_xdg_surface* prev_focused_xdg =
            wlr_xdg_surface_from_wlr_surface(prev_focused);
        wlr_xdg_toplevel_set_activated(prev_focused_xdg, false);
    }

    /* Move view to top. */
    wlr_scene_node_raise_to_top(bsi_view->wlr_scene_node);
    /* Add the view to the front of the list. */
    bsi_views_remove(bsi_views, bsi_view);
    bsi_views_add(bsi_views, bsi_view);
    /* Activate the view surface. */
    wlr_xdg_toplevel_set_activated(bsi_view->wlr_xdg_surface, true);
    /* Tell seat to enter this surface with the keyboard. Don't touch the
     * pointer. */
    struct wlr_keyboard* wlr_keyboard = wlr_seat_get_keyboard(wlr_seat);
    wlr_seat_keyboard_notify_enter(wlr_seat,
                                   bsi_view->wlr_xdg_surface->surface,
                                   wlr_keyboard->keycodes,
                                   wlr_keyboard->num_keycodes,
                                   &wlr_keyboard->modifiers);
}

void
bsi_view_add_listener(struct bsi_view* bsi_view,
                      enum bsi_view_listener_mask bsi_listener_type,
                      struct wl_listener* bsi_listener_memb,
                      struct wl_signal* bsi_signal_memb,
                      wl_notify_func_t func)
{
    assert(bsi_view);
    assert(func);

    bsi_listener_memb->notify = func;
    bsi_view->active_listeners |= bsi_listener_type;
    bsi_view->active_links[bsi_view->len_active_links++] =
        &bsi_listener_memb->link;

    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}

void
bsi_view_listener_unlink_all(struct bsi_view* bsi_view)
{
    assert(bsi_view);

    for (size_t i = 0; i < bsi_view->len_active_links; ++i) {
        if (bsi_view->active_links[i] != NULL)
            wl_list_remove(bsi_view->active_links[i]);
    }
}
