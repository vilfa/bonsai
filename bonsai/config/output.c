#include <assert.h>
#include <stdlib.h>
#include <wayland-util.h>

#include "bonsai/config/output.h"

struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs)
{
    assert(bsi_outputs);

    bsi_outputs->len = 0;
    wl_list_init(&(bsi_outputs->outputs));

    return bsi_outputs;
}
