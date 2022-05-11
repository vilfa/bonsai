#pragma once

#include <wayland-util.h>

#include "bonsai/config/output.h"
#include "bonsai/scene/view.h"

struct bsi_server;

/**
 * @brief Holds all workspaces that belong to the server.
 *
 */
struct bsi_workspaces
{
    // TODO: Maybe limiting the amount of workspaces isn't a bad idea?
    size_t len;
    struct wl_list workspaces;

    struct
    {
        struct wl_signal active;
    } events;
};

/**
 * @brief Workspace is the parent of `bsi_view`. Views are grouped by
 * workspaces.
 *
 */
struct bsi_workspace
{
    struct bsi_server* bsi_server;
    // TODO: Workspace spans outputs.
    struct bsi_output* bsi_output; /* Workspace spans outputs. */

    size_t id;   /* Incremental id. */
    char* name;  /* User given name. */
    bool active; /* A single workspace can be active at one time. */

    // TODO: Figure this out.
    size_t len_views;
    struct wl_list views;

    struct wl_list link;
};

/**
 * @brief Initializes a preallocated `bsi_workspaces`.
 *
 * @param bsi_workspaces The workspaces.
 * @return struct bsi_workspaces* The initialized workspaces.
 */
struct bsi_workspaces*
bsi_workspaces_init(struct bsi_workspaces* bsi_workspaces);

/**
 * @brief Add a workspace to the workspaces. This will emit `active` events for
 * all workspaces that changed their active state.
 *
 * @param bsi_workspaces The server workspaces.
 * @param bsi_workspace The workspace to add.
 */
void
bsi_workspaces_add(struct bsi_workspaces* bsi_workspaces,
                   struct bsi_workspace* bsi_workspace);

/**
 * @brief Remove a workspace from the workspaces. This will emit `active` events
 * for all workspaces that changed their active state.
 *
 * @param bsi_workspaces The server workspaces.
 * @param bsi_workspace The workspace to remove.
 */
void
bsi_workspaces_remove(struct bsi_workspaces* bsi_workspaces,
                      struct bsi_workspace* bsi_workspace);

/**
 * @brief Get the active workspace.
 *
 * @param bsi_workspaces The server workspaces.
 * @return struct bsi_workspace* The active workspace or NULL (shouldn't
 * happen).
 */
struct bsi_workspace*
bsi_workspaces_get_active(struct bsi_workspaces* bsi_workspaces);

/**
 * @brief Initialize a preallocated workspace.
 *
 * @param bsi_workspace The workspace to initialize.
 * @param bsi_server The server.
 * @param bsi_output The output to attach the workspace to.
 * @param name The workspace name.
 * @return struct bsi_workspace* The initialized workspace.
 */
struct bsi_workspace*
bsi_workspace_init(struct bsi_workspace* bsi_workspace,
                   struct bsi_server* bsi_server,
                   struct bsi_output* bsi_output,
                   const char* name);

/**
 * @brief Destroy the workspace. This calls `free` on all resources held by the
 * workspace.
 *
 * @param bsi_workspace The workspace to destroy.
 */
void
bsi_workspace_destroy(struct bsi_workspace* bsi_workspace);

/**
 * @brief Sets the workspace active state. Only a single workspace should be
 * active at a time.
 *
 * @param bsi_workspace The workspace.
 * @param active The active state.
 */
void
bsi_workspace_set_active(struct bsi_workspace* bsi_workspace, bool active);

/**
 * @brief Add a view to the workspace.
 *
 * @param bsi_workspace The workspace.
 * @param bsi_view The view to add.
 */
void
bsi_workspace_view_add(struct bsi_workspace* bsi_workspace,
                       struct bsi_view* bsi_view);

/**
 * @brief Remove a view from the workspace.
 *
 * @param bsi_workspace The workspace.
 * @param bsi_view The view to remove.
 */
void
bsi_workspace_view_remove(struct bsi_workspace* bsi_workspace,
                          struct bsi_view* bsi_view);

/**
 * @brief Move a view from one workspace to another.
 *
 * @param bsi_workspace_from Old workspace parent of view.
 * @param bsi_workspace_to New workspace parent of view.
 * @param bsi_view The view to move.
 */
void
bsi_workspace_view_move(struct bsi_workspace* bsi_workspace_from,
                        struct bsi_workspace* bsi_workspace_to,
                        struct bsi_view* bsi_view);
