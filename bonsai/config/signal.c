#include <assert.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "bonsai/config/signal.h"
#include "bonsai/server.h"

struct bsi_listeners*
bsi_listeners_init(struct bsi_listeners* bsi_listeners)
{
    assert(bsi_listeners);

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    bsi_listeners->bsi_server = bsi_server;
    bsi_listeners->active_listeners = 0;
    bsi_listeners->len_active_links = 0;
    return bsi_listeners;
}

void
bsi_listeners_add_listener(struct bsi_listeners* bsi_listeners,
                           enum bsi_listeners_mask bsi_listener_type,
                           struct wl_listener* bsi_listeners_memb,
                           struct wl_signal* bsi_signal_memb,
                           wl_notify_func_t func)
{
    assert(bsi_listeners);
    assert(bsi_listeners_memb);
    assert(bsi_signal_memb);
    assert(func);

    bsi_listeners_memb->notify = func;
    bsi_listeners->active_listeners |= bsi_listener_type;
    bsi_listeners->active_links[bsi_listeners->len_active_links++] =
        &bsi_listeners_memb->link;

    wl_signal_add(bsi_signal_memb, bsi_listeners_memb);
}

void
bsi_listeners_unlink_all(struct bsi_listeners* bsi_listeners)
{
    assert(bsi_listeners);

    for (size_t i = 0; i < bsi_listeners->len_active_links; ++i) {
        if (bsi_listeners->active_links[i] != NULL)
            wl_list_remove(bsi_listeners->active_links[i]);
    }
}
