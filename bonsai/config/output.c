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
    wl_list_remove(&bsi_output->destroy.link);
    wl_list_remove(&bsi_output->frame.link);
    free(bsi_output);
}

size_t
bsi_outputs_len(struct bsi_outputs* bsi_outputs)
{
    assert(bsi_outputs);

    return bsi_outputs->len;
}

void
bsi_output_add_destroy_listener(struct bsi_output* bsi_output,
                                wl_notify_func_t func)
{
    assert(bsi_output);

    bsi_output->destroy.notify = func;
    wl_signal_add(&bsi_output->wlr_output->events.destroy,
                  &bsi_output->destroy);
}

void
bsi_output_add_frame_listener(struct bsi_output* bsi_output,
                              wl_notify_func_t func)
{
    assert(bsi_output);

    bsi_output->frame.notify = func;
    wl_signal_add(&bsi_output->wlr_output->events.frame, &bsi_output->frame);
}
