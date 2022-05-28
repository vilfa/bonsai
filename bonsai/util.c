#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/util/log.h>

struct bsi_server;

#include "bonsai/log.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct timespec
bsi_util_timespec_get()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts;
}

void
bsi_util_slot_connect(struct wl_signal* bsi_signal_memb,
                      struct wl_listener* bsi_listener_memb,
                      wl_notify_func_t func)
{
    assert(bsi_listener_memb);
    assert(bsi_signal_memb);
    assert(func);

    bsi_listener_memb->notify = func;
    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}

void
bsi_util_slot_disconnect(struct wl_listener* bsi_listener_memb)
{
    assert(bsi_listener_memb);
    wl_list_remove(&bsi_listener_memb->link);
}

bool
bsi_util_forkexec(char* const* argp, const size_t len_argp)
{
    if (len_argp < 1)
        return false;

    switch (fork()) {
        case 0: {
            extern char** environ;

            execve(argp[0], argp, environ);

            bsi_log(WLR_ERROR, "Exec failed: %s", strerror(errno));
            _exit(EXIT_FAILURE);
            break;
        }
        case -1:
            bsi_log(WLR_ERROR, "Fork failed: %s", strerror(errno));
            return false;
        default:
            return true;
    }
}
