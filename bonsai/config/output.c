#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_output.h>

#include "bonsai/config/output.h"

struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs)
{
    assert(bsi_outputs);

    bsi_outputs->len = 0;
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
    wl_list_remove(&bsi_output->events.frame.link);
    wl_list_remove(&bsi_output->events.damage.link);
    wl_list_remove(&bsi_output->events.needs_frame.link);
    wl_list_remove(&bsi_output->events.precommit.link);
    wl_list_remove(&bsi_output->events.commit.link);
    wl_list_remove(&bsi_output->events.present.link);
    wl_list_remove(&bsi_output->events.bind.link);
    wl_list_remove(&bsi_output->events.enable.link);
    wl_list_remove(&bsi_output->events.mode.link);
    wl_list_remove(&bsi_output->events.description.link);
    wl_list_remove(&bsi_output->events.destroy.link);
    free(bsi_output);
}

size_t
bsi_outputs_len(struct bsi_outputs* bsi_outputs)
{
    assert(bsi_outputs);

    return bsi_outputs->len;
}

void
bsi_output_add_listener(struct bsi_output* bsi_output,
                        enum bsi_output_listener_mask listener_type,
                        struct wl_listener* bsi_listener_memb,
                        struct wl_signal* bsi_signal_memb,
                        wl_notify_func_t func)
{
    assert(bsi_output);
    assert(func);

    bsi_output->active_listeners |= listener_type;
    bsi_listener_memb->notify = func;
    wl_signal_add(bsi_signal_memb, bsi_listener_memb);
}
