#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>

#include "bonsai/config/output.h"
#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/workspace.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs, struct bsi_server* bsi_server)
{
    assert(bsi_outputs);
    assert(bsi_server);

    bsi_outputs->len = 0;
    bsi_outputs->bsi_server = bsi_server;
    wl_list_init(&bsi_outputs->outputs);

    return bsi_outputs;
}

void
bsi_outputs_add(struct bsi_outputs* bsi_outputs, struct bsi_output* bsi_output)
{
    assert(bsi_outputs);
    assert(bsi_output);

    ++bsi_outputs->len;
    wl_list_insert(&bsi_outputs->outputs, &bsi_output->link);
}

void
bsi_outputs_remove(struct bsi_outputs* bsi_outputs,
                   struct bsi_output* bsi_output)
{
    assert(bsi_outputs);
    assert(bsi_output);

    --bsi_outputs->len;
    wl_list_remove(&bsi_output->link);
    bsi_output_listeners_unlink_all(bsi_output);
}

struct bsi_output*
bsi_outputs_get_active(struct bsi_outputs* bsi_outputs)
{
    assert(bsi_outputs);

    struct bsi_server* bsi_server = bsi_outputs->bsi_server;
    struct wlr_cursor* cursor = bsi_server->wlr_cursor;

    struct wlr_output* active_output = wlr_output_layout_output_at(
        bsi_server->wlr_output_layout, cursor->x, cursor->y);

    struct bsi_output *first, *output;
    wl_list_for_each(output, &bsi_server->bsi_outputs.outputs, link)
    {
        if (first == NULL)
            first = output;

        if (output->wlr_output == active_output)
            return output;
    }

    return first;
}

struct bsi_output*
bsi_output_init(struct bsi_output* bsi_output,
                struct bsi_server* bsi_server,
                struct wlr_output* wlr_output,
                struct bsi_workspaces* bsi_workspaces,
                struct bsi_layers* bsi_layers)
{
    assert(bsi_output);
    assert(bsi_server);
    assert(wlr_output);
    assert(bsi_workspaces);
    assert(bsi_layers);

    bsi_output->id = bsi_server->bsi_outputs.len;
    bsi_output->active_listeners = 0;
    bsi_output->len_active_links = 0;
    bsi_output->bsi_server = bsi_server;
    bsi_output->wlr_output = wlr_output;
    bsi_output->bsi_workspaces = bsi_workspaces;
    bsi_output->bsi_layers = bsi_layers;

    struct timespec now = bsi_util_timespec_get();
    bsi_output->last_frame = now;

    return bsi_output;
}

void
bsi_output_destroy(struct bsi_output* bsi_output)
{
    assert(bsi_output);

    // TODO: Take care of workspaces and layers.
    free(bsi_output);
}

void
bsi_output_listener_add(struct bsi_output* bsi_output,
                        enum bsi_output_listener_mask bsi_listener_type,
                        struct wl_listener* bsi_listener_memb,
                        struct wl_signal* bsi_signal_memb,
                        wl_notify_func_t func)
{
    assert(bsi_output);
    assert(func);

    bsi_listener_memb->notify = func;
    bsi_output->active_listeners |= bsi_listener_type;
    bsi_output->active_links[bsi_output->len_active_links++] =
        &bsi_listener_memb->link;

    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}

void
bsi_output_listeners_unlink_all(struct bsi_output* bsi_output)
{
    assert(bsi_output);

    for (size_t i = 0; i < bsi_output->len_active_links; ++i) {
        if (bsi_output->active_links[i] != NULL)
            wl_list_remove(bsi_output->active_links[i]);
    }
}
