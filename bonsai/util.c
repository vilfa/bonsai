#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
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
bsi_util_slot_connect(struct wl_signal* signal_memb,
                      struct wl_listener* listener_memb,
                      wl_notify_func_t func)
{
    listener_memb->notify = func;
    wl_signal_add(signal_memb, listener_memb);
}

void
bsi_util_slot_disconnect(struct wl_listener* listener_memb)
{
    assert(listener_memb);
    wl_list_remove(&listener_memb->link);
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

            bsi_errno("Exec failed");
            _exit(EXIT_FAILURE);
            break;
        }
        case -1:
            bsi_errno("Fork failed");
            return false;
        default:
            return true;
    }
}

size_t
bsi_util_split_argsp(char* first, char* in, const char* delim, char*** out)
{
    if (*out != NULL)
        return 0;

    *out = realloc(*out, 2 * sizeof(char**));
    (*out)[0] = first;

    size_t len = 2;
    char* tok = strtok(in, delim);
    while (tok) {
        (*out)[len - 1] = tok;
        *out = realloc(*out, ++len * sizeof(char**));
        tok = strtok(NULL, delim);
    }

    (*out)[len - 1] = NULL;

    return len;
}

size_t
bsi_util_split_delim(char* in, const char* delim, char*** out)
{
    if (*out != NULL)
        return 0;

    *out = realloc(*out, sizeof(char**));

    size_t len = 1;
    char* tok = strtok(in, delim);
    while (tok) {
        (*out)[len - 1] = tok;
        *out = realloc(*out, ++len * sizeof(char**));
        tok = strtok(NULL, delim);
    }

    (*out)[len - 1] = NULL;

    return len;
}

void
bsi_util_split_free(char*** out)
{
    free(*out);
}
