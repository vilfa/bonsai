#include <stdlib.h>
#include <wayland-util.h>

#include "bonsai/desktop/lock.h"
#include "bonsai/events.h"
#include "bonsai/input.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/util.h"

void
bsi_session_locks_add(struct bsi_server* server, struct bsi_session_lock* lock)
{
    wl_list_insert(&server->session.locks, &lock->link_server);
}

void
bsi_session_locks_remove(struct bsi_session_lock* lock)
{
    wl_list_remove(&lock->link_server);
}

struct bsi_session_lock*
bsi_session_lock_init(struct bsi_session_lock* lock,
                      struct bsi_server* server,
                      struct wlr_session_lock_v1* wlr_lock)
{
    lock->lock = wlr_lock;
    lock->server = server;
    wl_list_init(&lock->surfaces);
    return lock;
}

void
bsi_session_lock_destroy(struct bsi_session_lock* lock)
{
    wl_list_remove(&lock->listen.new_surface.link);
    wl_list_remove(&lock->listen.unlock.link);
    wl_list_remove(&lock->listen.destroy.link);
    free(lock);
}

struct bsi_session_lock_surface*
bsi_session_lock_surface_init(
    struct bsi_session_lock_surface* surface,
    struct bsi_session_lock* lock,
    struct wlr_session_lock_surface_v1* wlr_lock_surface)
{
    surface->lock_surface = wlr_lock_surface;
    surface->lock = lock;
    surface->surface = wlr_lock_surface->surface;

    surface->scene_surface = wlr_scene_surface_create(
        &lock->server->wlr_scene->node, surface->surface);
    surface->scene_surface->surface->data = surface;

    return surface;
}

void
bsi_session_lock_surface_destroy(struct bsi_session_lock_surface* surface)
{
    wl_list_remove(&surface->listen.map.link);
    wl_list_remove(&surface->listen.destroy.link);
    wl_list_remove(&surface->listen.surface_commit.link);
    wl_list_remove(&surface->listen.mode.link);
    wl_list_remove(&surface->listen.output_commit.link);
    free(surface);
}

/*
 * Handlers
 */
/* bsi_session_lock */
void
handle_session_lock_new_surface(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event new_surface from bsi_session_lock");

    struct bsi_session_lock* lock =
        wl_container_of(listener, lock, listen.new_surface);
    struct wlr_session_lock_surface_v1* surface = data;
    struct bsi_server* server = lock->server;

    struct bsi_session_lock_surface* lock_surface =
        calloc(1, sizeof(struct bsi_session_lock_surface));
    bsi_session_lock_surface_init(lock_surface, lock, surface);

    bsi_util_slot_connect(&surface->events.map,
                          &lock_surface->listen.map,
                          handle_lock_surface_map);
    bsi_util_slot_connect(&surface->events.destroy,
                          &lock_surface->listen.destroy,
                          handle_lock_surface_destroy);
    bsi_util_slot_connect(&surface->surface->events.commit,
                          &lock_surface->listen.surface_commit,
                          handle_lock_surface_surface_commit);
    bsi_util_slot_connect(&surface->output->events.mode,
                          &lock_surface->listen.mode,
                          handle_lock_surface_output_mode);
    bsi_util_slot_connect(&surface->output->events.commit,
                          &lock_surface->listen.output_commit,
                          handle_lock_surface_output_commit);

    wl_list_insert(&lock->surfaces, &lock_surface->link_session_lock);

    wlr_session_lock_surface_v1_configure(
        surface, surface->output->width, surface->output->height);

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        bsi_output_surface_damage(output, NULL, true);
    }
}

void
handle_session_lock_unlock(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event unlock from bsi_session_lock");

    struct bsi_session_lock* lock =
        wl_container_of(listener, lock, listen.unlock);
    struct bsi_server* server = lock->server;

    server->session.locked = false;

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        bsi_output_surface_damage(output, NULL, true);
    }
}

void
handle_session_lock_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from bsi_sesion_lock");

    struct bsi_session_lock* lock =
        wl_container_of(listener, lock, listen.destroy);
    struct bsi_server* server = lock->server;

    bsi_session_locks_remove(lock);
    bsi_session_lock_destroy(lock);

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        bsi_output_surface_damage(output, NULL, true);
    }
}

/* bsi_session_lock_surface */
void
handle_lock_surface_map(struct wl_listener* listener, void* data)
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
        bsi_output_surface_damage(output, NULL, true);
    }
}

void
handle_lock_surface_destroy(struct wl_listener* listener, void* data)
{
    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.destroy);

    wl_list_remove(&lock_surface->link_session_lock);
    bsi_session_lock_surface_destroy(lock_surface);
}

void
handle_lock_surface_surface_commit(struct wl_listener* listener, void* data)
{
    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.surface_commit);
    struct bsi_server* server = lock_surface->lock->server;

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        bsi_output_surface_damage(output, NULL, true);
    }
}

void
handle_lock_surface_output_mode(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event mode from wlr_output of wlr_session_lock_surface");

    struct bsi_session_lock_surface* lock_surface =
        wl_container_of(listener, lock_surface, listen.mode);
    wlr_session_lock_surface_v1_configure(
        lock_surface->lock_surface,
        lock_surface->lock_surface->output->width,
        lock_surface->lock_surface->output->height);
}

void
handle_lock_surface_output_commit(struct wl_listener* listener, void* data)
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
