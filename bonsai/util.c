#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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
    wl_list_remove(&listener_memb->link);
}

bool
bsi_util_tryexec(char* const* argp, const size_t len_argp)
{
    const char* binpaths[] = { "/usr/bin/", "/usr/local/bin/" };
    char fargp[50] = { 0 };
    char* aargp[len_argp];
    for (size_t i = 0; i < 2; ++i) {
        snprintf(fargp, 50, "%s%s", binpaths[i], argp[0]);
        aargp[0] = fargp;
        for (size_t i = 1; i < len_argp; ++i) {
            aargp[i] = argp[i];
        }
        bsi_info("Trying to exec '%s'", fargp);
        if (access(fargp, F_OK | X_OK) == 0) {
            return bsi_util_forkexec(aargp, len_argp);
        } else {
            bsi_errno("File '%s' F_OK | X_OK != 0", fargp);
        }
        memset(fargp, 0, 50);
    }
    return false;
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

            bsi_errno("Exec '%s' failed", argp[0]);
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
bsi_util_split_delim(char* in,
                     const char* delim,
                     char*** out,
                     bool ignore_quotes)
{
    if (*out != NULL)
        return 0;

    *out = realloc(*out, sizeof(char**));

    enum state
    {
        TOK_BEGIN,
        TOK_QUOTED,
        TOK_IN,
        TOK_END,
        DONE,
    };

    size_t end = strlen(in) + 1;
    size_t cursor = 0;
    size_t len = 1;
    char* ltok = 0;
    char qsign = 0;
    enum state state = TOK_BEGIN;
    while (cursor < end) {
        switch (state) {
            case TOK_BEGIN: {
                if (in[cursor] == '\0') {
                    state = DONE;
                    ++end;
                } else if ((in[cursor] == '\'' || in[cursor] == '"') &&
                           !ignore_quotes) {
                    ltok = in + cursor;
                    qsign = in[cursor];
                    state = TOK_QUOTED;
                } else if (in[cursor] != *delim) {
                    ltok = in + cursor;
                    state = TOK_IN;
                }
                ++cursor;
                break;
            }
            case TOK_QUOTED: {
                if (in[cursor] == qsign) {
                    state = TOK_IN;
                }
                ++cursor;
                break;
            }
            case TOK_IN: {
                if (in[cursor] == *delim) {
                    state = TOK_END;
                    in[cursor] = '\0';
                } else if (in[cursor] == '\0') {
                    state = TOK_END;
                    ++end;
                }
                ++cursor;
                break;
            }
            case TOK_END: {
                (*out)[len - 1] = ltok;
                ++len;
                *out = realloc(*out, len * sizeof(char**));
                state = TOK_BEGIN;
                break;
            }
            case DONE: {
                (*out)[len - 1] = NULL;
                ++cursor;
                break;
            }
        }
    }

    char tokens[1024] = { 0 };
    tokens[0] = '[';
    for (size_t i = 0; i < len - 1; ++i) {
        if (strlen(tokens) + strlen((*out)[i] + 1) > 1023) {
            strncpy(tokens, (*out)[i], 1024 - strlen(tokens) - 1);
            tokens[1023] = '\0';
        }
        sprintf(tokens, "%s%s,", tokens, (*out)[i]);
    }
    tokens[strlen(tokens) - 1] = ']';
    tokens[strlen(tokens)] = '\0';

    bsi_debug("Tokens { len=%ld, tokens=%s }", len, tokens);

    return len;
}

void
bsi_util_split_free(char*** out)
{
    free(*out);
}

void
bsi_util_strip_quotes(char* in)
{
    if (strlen(in) < 1)
        return;

    size_t first_non = strspn(in, "'\""),
           last_non = strcspn(in + first_non, "'\"") + first_non,
           len_non = last_non - first_non, len = strlen(in);

    if (len_non == len)
        return;

    memset(in, 0, first_non);
    memset(in + last_non, 0, len - last_non);
    memmove(in, in + first_non, len_non);
    memset(in + len_non, 0, len - len_non);
}
