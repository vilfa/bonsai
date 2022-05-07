#include <assert.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "bonsai/config/signal.h"
#include "bonsai/server.h"

// TODO: Maybe think about adding a generic function for adding these listeners,
// maybe something like this:
// void bsi_listeners_add_listener(struct bsi_listeners* bsi_listeners,
//                                 uint32_t listener_type,
//                                 wl_listener* bsi_listener_memb,
//                                 wl_signal* bsi_signal_memb,
//                                 wl_notify_func_t func)
// or maybe something more robust if you can think of it.

struct bsi_listeners*
bsi_listeners_init(struct bsi_listeners* bsi_listeners)
{
    assert(bsi_listeners);

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    bsi_listeners->bsi_server = bsi_server;
    bsi_listeners->active_listeners = 0;
    return bsi_listeners;
}

void
bsi_listeners_add_new_output_notify(struct bsi_listeners* bsi_listeners,
                                    wl_notify_func_t func)
{
    assert(bsi_listeners);
    assert(func);

    bsi_listeners->active_listeners |= BSI_LISTENER_OUTPUT;
    bsi_listeners->new_output.notify = func;

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    wl_signal_add(&bsi_server->wlr_backend->events.new_output,
                  &bsi_listeners->new_output);
}

void
bsi_listeners_add_request_cursor_notify(struct bsi_listeners* bsi_listeners,
                                        wl_notify_func_t func)
{
    assert(bsi_listeners);
    assert(func);

    bsi_listeners->active_listeners |= BSI_LISTENER_REQUEST_SET_CURSOR;
    bsi_listeners->request_set_cursor.notify = func;

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    wl_signal_add(&bsi_server->wlr_seat->events.request_set_cursor,
                  &bsi_listeners->request_set_cursor);
}

void
bsi_listeners_add_request_set_selection_notify(
    struct bsi_listeners* bsi_listeners,
    wl_notify_func_t func)
{
    assert(bsi_listeners);
    assert(func);

    bsi_listeners->active_listeners |= BSI_LISTENER_REQUEST_SET_SELECTION;
    bsi_listeners->request_set_selection.notify = func;

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    wl_signal_add(&bsi_server->wlr_seat->events.request_set_selection,
                  &bsi_listeners->request_set_selection);
}

void
bsi_listeners_add_new_input_notify(struct bsi_listeners* bsi_listeners,
                                   wl_notify_func_t func)
{
    assert(bsi_listeners);
    assert(func);

    bsi_listeners->active_listeners |= BSI_LISTENER_INPUT;
    bsi_listeners->new_input.notify = func;

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    wl_signal_add(&bsi_server->wlr_backend->events.new_input,
                  &bsi_listeners->new_input);
}

void
bsi_listeners_add_new_xdg_surface_notify(struct bsi_listeners* bsi_listeners,
                                         wl_notify_func_t func)
{
    assert(bsi_listeners);
    assert(func);

    bsi_listeners->active_listeners |= BSI_LISTENER_XDG_SURFACE;
    bsi_listeners->new_xdg_surface.notify = func;

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners, bsi_server, bsi_listeners);

    wl_signal_add(&bsi_server->wlr_xdg_shell->events.new_surface,
                  &bsi_listeners->new_xdg_surface);
}
