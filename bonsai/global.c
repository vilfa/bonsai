#include <assert.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "bonsai/desktop/view.h"
#include "bonsai/global.h"
#include "bonsai/server.h"

struct bsi_listeners_global*
bsi_listeners_global_init(struct bsi_listeners_global* bsi_listeners_global)
{
    assert(bsi_listeners_global);

    struct bsi_server* bsi_server =
        wl_container_of(bsi_listeners_global, bsi_server, bsi_listeners_global);

    bsi_listeners_global->bsi_server = bsi_server;
    bsi_listeners_global->len_active_listen = 0;
    return bsi_listeners_global;
}

void
bsi_listeners_global_add(struct bsi_listeners_global* bsi_listeners_global,
                         struct wl_listener* bsi_listeners_memb,
                         struct wl_signal* bsi_signal_memb,
                         wl_notify_func_t func)
{
    assert(bsi_listeners_global);
    assert(bsi_listeners_memb);
    assert(bsi_signal_memb);
    assert(func);

    bsi_listeners_memb->notify = func;
    ++bsi_listeners_global->len_active_listen;
    wl_signal_add(bsi_signal_memb, bsi_listeners_memb);
}

void
bsi_listeners_global_finish(struct bsi_listeners_global* bsi_listeners_global)
{
    assert(bsi_listeners_global);

    /* wlr_backend */
    wl_list_remove(&bsi_listeners_global->listen.wlr_backend_new_output.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_backend_new_input.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_backend_destroy.link);
    /* wlr_seat */
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_pointer_grab_begin.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_pointer_grab_end.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_keyboard_grab_begin.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_keyboard_grab_end.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_touch_grab_begin.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_seat_touch_grab_end.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_request_set_cursor.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_request_set_selection.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_seat_set_selection.link);
    wl_list_remove(&bsi_listeners_global->listen
                        .wlr_seat_request_set_primary_selection.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_set_primary_selection.link);
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_seat_request_start_drag.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_seat_start_drag.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_seat_destroy.link);
    /* wlr_xdg_shell */
    wl_list_remove(
        &bsi_listeners_global->listen.wlr_xdg_shell_new_surface.link);
    wl_list_remove(&bsi_listeners_global->listen.wlr_xdg_shell_destroy.link);
    /* bsi_workspace */
    wl_list_remove(&bsi_listeners_global->listen.bsi_workspace_active.link);

    bsi_listeners_global->len_active_listen = 0;
}
