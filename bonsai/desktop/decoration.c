#include <assert.h>
#include <stdlib.h>
#include <wayland-util.h>

#include "bonsai/desktop/decoration.h"

void
bsi_scene_add_decoration(struct bsi_server* server,
                         struct bsi_server_decoration* deco)
{
    ++server->scene.len_decorations;
    wl_list_insert(&server->scene.decorations, &deco->link);
}

void
bsi_scene_remove_decoration(struct bsi_server* server,
                            struct bsi_server_decoration* deco)
{
    --server->scene.len_decorations;
    wl_list_remove(&deco->link);
}

struct bsi_server_decoration*
bsi_server_decoration_init(struct bsi_server_decoration* deco,
                           struct bsi_server* server,
                           struct wlr_server_decoration* wlr_server_deco)
{
    deco->server = server;
    deco->server_decoration = wlr_server_deco;
    return deco;
}

void
bsi_server_decoration_finish(struct bsi_server_decoration* deco)
{
    wl_list_remove(&deco->listen.destroy.link);
    wl_list_remove(&deco->listen.mode.link);
}

void
bsi_server_decoration_destroy(struct bsi_server_decoration* deco)
{
    free(deco);
}
