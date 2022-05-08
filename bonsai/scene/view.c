#include <assert.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "bonsai/scene/view.h"
#include "bonsai/server.h"

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
