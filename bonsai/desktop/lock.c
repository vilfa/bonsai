#include <assert.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_session_lock_v1.h>

#include "bonsai/desktop/lock.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/util.h"

struct bsi_session_lock*
session_lock_init(struct bsi_session_lock* lock,
                  struct bsi_server* server,
                  struct wlr_session_lock_v1* wlr_lock)
{
    lock->lock = wlr_lock;
    lock->server = server;
    lock->tree = wlr_scene_tree_create(&server->wlr_scene->node);
    wl_list_init(&lock->surfaces);
    return lock;
}

void
session_lock_destroy(struct bsi_session_lock* lock)
{
    wl_list_remove(&lock->listen.new_surface.link);
    wl_list_remove(&lock->listen.unlock.link);
    wl_list_remove(&lock->listen.destroy.link);
    free(lock);
}

struct bsi_session_lock_surface*
session_lock_surface_init(struct bsi_session_lock_surface* surface,
                          struct bsi_session_lock* lock,
                          struct wlr_session_lock_surface_v1* wlr_lock_surface)
{
    surface->lock_surface = wlr_lock_surface;
    surface->lock = lock;
    surface->surface = wlr_lock_surface->surface;
    return surface;
}

void
session_lock_surface_destroy(struct bsi_session_lock_surface* surface)
{
    wl_list_remove(&surface->listen.map.link);
    wl_list_remove(&surface->listen.destroy.link);
    wl_list_remove(&surface->listen.surface_commit.link);
    wl_list_remove(&surface->listen.mode.link);
    wl_list_remove(&surface->listen.output_commit.link);
    free(surface);
}

/* Handlers */
/* bsi_session_lock_surface */
static void
handle_surface_map(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event map from wlr_session_lock_surface");

    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.map);
    struct bsi_server* server = lock_surface->lock->server;
    struct wlr_seat* seat = server->wlr_seat;

    struct bsi_input_device* dev;
    wl_list_for_each(dev, &server->input.inputs, link_server)
    {
        switch (dev->type) {
            case BSI_INPUT_DEVICE_POINTER:
                wlr_seat_pointer_clear_focus(seat);
                wlr_seat_pointer_notify_enter(
                    seat, lock_surface->surface, 0.0, 0.0);
                break;
            case BSI_INPUT_DEVICE_KEYBOARD:
                wlr_seat_keyboard_clear_focus(seat);
                wlr_seat_keyboard_notify_enter(
                    seat, lock_surface->surface, NULL, 0, NULL);
                break;
            default:
                break;
        }
    }

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}

static void
handle_surface_destroy(struct wl_listener* listener, void* data)
{
    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.destroy);

    wl_list_remove(&lock_surface->link_session_lock);
    session_lock_surface_destroy(lock_surface);
}

static void
handle_surface_commit(struct wl_listener* listener, void* data)
{
    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.surface_commit);
    struct bsi_server* server = lock_surface->lock->server;

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}

static void
handle_output_mode(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event mode from wlr_output of wlr_session_lock_surface");

    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.mode);
    wlr_session_lock_surface_v1_configure(
        lock_surface->lock_surface,
        lock_surface->lock_surface->output->width,
        lock_surface->lock_surface->output->height);
}

static void
handle_output_commit(struct wl_listener* listener, void* data)
{
    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.output_commit);
    struct wlr_output_event_commit* event = data;
    if (event->committed & (WLR_OUTPUT_STATE_MODE | WLR_OUTPUT_STATE_SCALE |
                            WLR_OUTPUT_STATE_TRANSFORM)) {
        wlr_session_lock_surface_v1_configure(
            lock_surface->lock_surface,
            lock_surface->lock_surface->output->width,
            lock_surface->lock_surface->output->height);
    }
}

/* bsi_session_lock */
static void
handle_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from bsi_session_lock");

    struct bsi_session_lock* lock =
        wl_container_of(listener, lock, listen.new_surface);
    struct wlr_session_lock_surface_v1* surface = data;
    struct bsi_server* server = lock->server;

    struct bsi_session_lock_surface* lock_surface =
        calloc(1, sizeof(struct bsi_session_lock_surface));
    session_lock_surface_init(lock_surface, lock, surface);

    util_slot_connect(
        &surface->events.map, &lock_surface->listen.map, handle_surface_map);
    util_slot_connect(&surface->events.destroy,
                      &lock_surface->listen.destroy,
                      handle_surface_destroy);
    util_slot_connect(&surface->surface->events.commit,
                      &lock_surface->listen.surface_commit,
                      handle_surface_commit);
    util_slot_connect(&surface->output->events.mode,
                      &lock_surface->listen.mode,
                      handle_output_mode);
    util_slot_connect(&surface->output->events.commit,
                      &lock_surface->listen.output_commit,
                      handle_output_commit);

    wl_list_insert(&lock->surfaces, &lock_surface->link_session_lock);

    wlr_scene_subsurface_tree_create(&lock->tree->node, surface->surface);

    wlr_session_lock_surface_v1_configure(
        surface, surface->output->width, surface->output->height);

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}

static void
handle_unlock(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unlock from bsi_session_lock");

    struct bsi_session_lock* lock =
        wl_container_of(listener, lock, listen.unlock);
    struct bsi_server* server = lock->server;

    server->session.locked = false;
    bsi_info("Session unlocked");

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}

static void
handle_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from bsi_sesion_lock");

    struct bsi_session_lock* lock =
        wl_container_of(listener, lock, listen.destroy);
    struct bsi_server* server = lock->server;

    server->session.lock = NULL;
    session_lock_destroy(lock);

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}

/* Global server handlers. */
void
handle_session_lock_manager_new_lock(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_lock from wlr_session_lock_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_lock);
    struct wlr_session_lock_v1* lock = data;

    if (server->session.lock != NULL) {
        wlr_session_lock_v1_destroy(lock);
        return;
    }

    server->session.locked = true;
    bsi_info("Session locked");

    struct bsi_session_lock* session_lock =
        calloc(1, sizeof(struct bsi_session_lock));
    session_lock_init(session_lock, server, lock);

    util_slot_connect(&lock->events.new_surface,
                      &session_lock->listen.new_surface,
                      handle_new_surface);
    util_slot_connect(
        &lock->events.unlock, &session_lock->listen.unlock, handle_unlock);
    util_slot_connect(
        &lock->events.destroy, &session_lock->listen.destroy, handle_destroy);

    server->session.lock = session_lock;

    wlr_session_lock_v1_send_locked(session_lock->lock);

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        output_surface_damage(output, NULL, true);
    }
}
