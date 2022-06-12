#include <assert.h>
#include <cairo/cairo.h>
#include <drm_fourcc.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/backend/drm.h>
#include <wlr/render/allocator.h>
#include <wlr/render/dmabuf.h>
#include <wlr/render/drm_format_set.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/box.h>

#include "bonsai/desktop/decoration.h"
#include "bonsai/desktop/view.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/server.h"
#include "bonsai/util.h"

struct bsi_xdg_decoration*
decoration_init(struct bsi_xdg_decoration* deco,
                struct bsi_server* server,
                struct bsi_view* view,
                struct wlr_xdg_toplevel_decoration_v1* wlr_deco)
{
    deco->server = server;
    deco->view = view;
    deco->xdg_decoration = wlr_deco;
    deco->view->decoration_mode = wlr_deco->requested_mode;
    deco->view->decoration = deco;
    return deco;
}

void
decoration_update(struct bsi_xdg_decoration* decoration)
{
    struct bsi_view* view = decoration->view;
    bsi_debug("Update decoration for view '%s'",
              view->wlr_xdg_toplevel->app_id);
    wlr_scene_node_set_enabled(&decoration->scene_rect->node, view->mapped);
    wlr_scene_rect_set_size(
        decoration->scene_rect, view->geom.width + 4, view->geom.height + 4);
    wlr_scene_node_set_position(&decoration->scene_rect->node, -2, -2);
    wlr_scene_node_lower_to_bottom(&decoration->scene_rect->node);
    const float color_inactive[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    const float color_active[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool focused = views_get_focused(view->server) == view;
    wlr_scene_rect_set_color(decoration->scene_rect,
                             focused ? color_active : color_inactive);
    // wlr_scene_node_for_each_buffer(
    // &decoration->scene_rect->node, bsi_decoration_iter, decoration);
}

void
decoration_destroy(struct bsi_xdg_decoration* deco)
{
    wl_list_remove(&deco->listen.destroy.link);
    wl_list_remove(&deco->listen.request_mode.link);
    free(deco);
}

void
decoration_iter(struct wlr_scene_buffer* buffer,
                int sx,
                int sy,
                void* user_data)
{
    bsi_info("Hello decoration buffer");
    struct bsi_xdg_decoration* deco = user_data;
    struct bsi_view* view = deco->view;

    if (&deco->scene_rect->node != &buffer->node)
        return;

    struct wlr_box vb;
    wlr_xdg_surface_get_geometry(view->wlr_xdg_toplevel->base, &vb);

    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, vb.width + 5, vb.height + 5);
    cairo_t* cr = cairo_create(surface);
    cairo_set_line_width(cr, 2.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0.0, 0.0, vb.width + 3, vb.height + 3);
    cairo_stroke(cr);

    cairo_text_extents_t te;
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(
        cr, "SansSerif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 1.5);
    cairo_text_extents(cr, view->wlr_xdg_toplevel->app_id, &te);
    cairo_move_to(cr, vb.width / 2.0 - te.width / 2.0, 2.0);
    cairo_show_text(cr, view->wlr_xdg_toplevel->app_id);

    size_t isize = cairo_image_surface_get_stride(surface) *
                   cairo_image_surface_get_height(surface);

    void* data;
    uint32_t format;
    size_t stride;
    if (wlr_buffer_begin_data_ptr_access(buffer->buffer,
                                         WLR_BUFFER_DATA_PTR_ACCESS_WRITE,
                                         &data,
                                         &format,
                                         &stride)) {
        memcpy(data, cairo_image_surface_get_data(surface), isize);
        wlr_buffer_end_data_ptr_access(buffer->buffer);
    } else {
        bsi_error("FUUUUCCCCCKKKKK");
    }

    cairo_surface_destroy(surface);
    cairo_destroy(cr);
}

/* Handlers */
static void
handle_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_server_decoration");
    struct bsi_xdg_decoration* server_deco =
        wl_container_of(listener, server_deco, listen.destroy);
    decorations_remove(server_deco);
    decoration_destroy(server_deco);
}

static void
handle_request_mode(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_mode from wlr_server_decoration");
    struct bsi_xdg_decoration* server_deco =
        wl_container_of(listener, server_deco, listen.request_mode);
    struct wlr_xdg_toplevel_decoration_v1* deco = data;
    struct bsi_view* view = deco->surface->data;

    bsi_info("View with app-id '%s', requested SSD %d",
             view->wlr_xdg_toplevel->app_id,
             deco->requested_mode ==
                 WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    wlr_xdg_toplevel_decoration_v1_set_mode(deco, deco->requested_mode);
}

/* Global server handlers. */
void
handle_xdg_decoration_manager_new_decoration(struct wl_listener* listener,
                                             void* data)
{
    bsi_debug("Got event new_decoration from wlr_decoration_manager");

    struct bsi_server* server =
        wl_container_of(listener, server, listen.new_decoration);
    struct wlr_xdg_toplevel_decoration_v1* toplevel_deco = data;

    struct bsi_view* view =
        ((struct wlr_scene_node*)toplevel_deco->surface->data)->data;
    assert(view);

    struct bsi_xdg_decoration* xdg_deco =
        calloc(1, sizeof(struct bsi_xdg_decoration));
    decoration_init(xdg_deco, server, view, toplevel_deco);

    util_slot_connect(&toplevel_deco->events.destroy,
                      &xdg_deco->listen.destroy,
                      handle_destroy);
    util_slot_connect(&toplevel_deco->events.request_mode,
                      &xdg_deco->listen.request_mode,
                      handle_request_mode);

    decorations_add(server, xdg_deco);

    // bsi_decoration_draw(xdg_deco);

    // TODO: Until server side decorations are implemented, refuse to set SSD.
    wlr_xdg_toplevel_decoration_v1_set_mode(
        toplevel_deco, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
    // wlr_xdg_toplevel_decoration_v1_set_mode(toplevel_deco,
    // toplevel_deco->requested_mode);
}
