#include <assert.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend.h>

#include "bonsai/config/signal.h"
#include "bonsai/server.h"

struct bsi_listeners*
bsi_listeners_init(struct bsi_listeners* bsi_listeners,
                   struct bsi_server* bsi_server)
{
    assert(bsi_listeners);

    bsi_listeners->server = bsi_server;
    bsi_listeners->active_listeners = 0;
    return bsi_listeners;
}

void
bsi_listeners_add_new_output_notify(struct bsi_listeners* bsi_listeners,
                                    wl_notify_func_t func)
{
    assert(bsi_listeners);

    bsi_listeners->active_listeners |= BSI_LISTENER_OUTPUT;
    bsi_listeners->new_output_listener.notify = func;

    struct bsi_server* server =
        wl_container_of(bsi_listeners, server, bsi_listeners);

    wl_signal_add(&server->wlr_backend->events.new_output,
                  &bsi_listeners->new_output_listener);
}

void
bsi_listeners_add_new_input_notify(struct bsi_listeners* bsi_listeners,
                                   wl_notify_func_t func)
{
    assert(bsi_listeners);

    bsi_listeners->active_listeners |= BSI_LISTENER_INPUT;
    bsi_listeners->new_input_listener.notify = func;

    struct bsi_server* server =
        wl_container_of(bsi_listeners, server, bsi_listeners);

    wl_signal_add(&server->wlr_backend->events.new_input,
                  &bsi_listeners->new_input_listener);
}
