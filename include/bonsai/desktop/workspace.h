#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

struct bsi_server;
struct bsi_output;
struct bsi_view;

#include "bonsai/desktop/view.h"

/**
 * @brief Workspace is the parent of `bsi_view`. Views are grouped by
 * workspaces.
 *
 */
struct bsi_workspace
{
    struct bsi_server* server;
    struct bsi_output* output; /* Workspace belongs to a single output. */

    size_t id;   /* Incremental id. */
    char* name;  /* User given name. */
    bool active; /* A single workspace can be active at one time per output. */

    struct wl_list views; /* All views that belong to this workspace. */

    /* The workspace owns the listeners of the server and output, so it can also
     * take care of them when destroying itself. */
    struct bsi_workspace_listener* foreign_listeners; /* len = 2 */

    struct
    {
        struct wl_signal active;
    } signal;

    struct wl_list link_output; // bsi_output
};

struct bsi_workspace_listener
{
    struct wl_listener active;
    struct wl_list link; // bsi_server::listen::workspace_active,
                         // bsi_output::listen::workspace_active
};

struct bsi_workspace*
workspace_init(struct bsi_workspace* workspace,
               struct bsi_server* server,
               struct bsi_output* output,
               const char* name);

void
workspace_destroy(struct bsi_workspace* workspace);

size_t
workspace_get_global_id(struct bsi_workspace* workspace);

void
workspace_set_active(struct bsi_workspace* workspace, bool active);

void
workspace_view_add(struct bsi_workspace* workspace, struct bsi_view* view);

void
workspace_view_remove(struct bsi_workspace* workspace, struct bsi_view* view);

void
workspace_view_move(struct bsi_workspace* workspace_from,
                    struct bsi_workspace* workspace_to,
                    struct bsi_view* view);

void
workspace_views_show_all(struct bsi_workspace* workspace, bool show_all);
