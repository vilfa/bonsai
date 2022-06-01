#pragma once

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

    struct
    {
        struct wl_signal active;
    } signal;

    struct wl_list link_output; // bsi_output
};

/**
 * @brief Add a workspace to the workspaces. This will emit `active` events for
 * all workspaces that changed their active state.
 *
 * @param output The output.
 * @param workspace The workspace to add.
 */
void
bsi_workspaces_add(struct bsi_output* output, struct bsi_workspace* workspace);

/**
 * @brief Remove a workspace from the workspaces. This will emit `active` events
 * for all workspaces that changed their active state.
 *
 * @param output The output.
 * @param workspace The workspace to remove.
 */
void
bsi_workspaces_remove(struct bsi_output* output,
                      struct bsi_workspace* workspace);

/**
 * @brief Get the active workspace.
 *
 * @param output The output.
 * @return struct bsi_workspace* The active workspace or NULL (shouldn't
 * happen).
 */
struct bsi_workspace*
bsi_workspaces_get_active(struct bsi_output* output);

/**
 * @brief Initialize a preallocated workspace.
 *
 * @param workspace The workspace to initialize.
 * @param server The server.
 * @param output The output to attach the workspace to.
 * @param name The workspace name.
 * @return struct bsi_workspace* The initialized workspace.
 */
struct bsi_workspace*
bsi_workspace_init(struct bsi_workspace* workspace,
                   struct bsi_server* server,
                   struct bsi_output* output,
                   const char* name);

/**
 * @brief Destroy the workspace. This calls `free` on all resources held by the
 * workspace.
 *
 * @param workspace The workspace to destroy.
 */
void
bsi_workspace_destroy(struct bsi_workspace* workspace);

/**
 * @brief Computes a unique id for the workspace.
 *
 * @param workspace The workspace.
 * @return size_t The unique global id.
 */
size_t
bsi_workspace_get_global_id(struct bsi_workspace* workspace);

/**
 * @brief Sets the workspace active state. Only a single workspace should be
 * active at a time.
 *
 * @param workspace The workspace.
 * @param active The active state.
 */
void
bsi_workspace_set_active(struct bsi_workspace* workspace, bool active);

/**
 * @brief Add a view to the workspace.
 *
 * @param workspace The workspace.
 * @param view The view to add.
 */
void
bsi_workspace_view_add(struct bsi_workspace* workspace, struct bsi_view* view);

/**
 * @brief Remove a view from the workspace.
 *
 * @param workspace The workspace.
 * @param view The view to remove.
 */
void
bsi_workspace_view_remove(struct bsi_workspace* workspace,
                          struct bsi_view* view);

/**
 * @brief Move a view from one workspace to another.
 *
 * @param workspace_from Old workspace parent of view.
 * @param workspace_to New workspace parent of view.
 * @param view The view to move.
 */
void
bsi_workspace_view_move(struct bsi_workspace* workspace_from,
                        struct bsi_workspace* workspace_to,
                        struct bsi_view* view);
