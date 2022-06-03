#include <cairo/cairo.h>
#include <drm_fourcc.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/box.h>

#include "bonsai/desktop/decoration.h"
#include "bonsai/log.h"
#include "bonsai/output.h"

void
bsi_decorations_add(struct bsi_server* server, struct bsi_decoration* deco)
{
    wl_list_insert(&server->scene.decorations, &deco->link_server);
}

void
bsi_decorations_remove(struct bsi_decoration* deco)
{
    wl_list_remove(&deco->link_server);
}

struct bsi_decoration*
bsi_decoration_init(struct bsi_decoration* deco,
                    struct bsi_server* server,
                    struct bsi_view* view,
                    struct wlr_xdg_toplevel_decoration_v1* wlr_deco)
{
    deco->server = server;
    deco->view = view;
    deco->toplevel_decoration = wlr_deco;
    deco->view->decoration_mode = wlr_deco->requested_mode;
    return deco;
}

void
bsi_decoration_destroy(struct bsi_decoration* deco)
{
    wl_list_remove(&deco->listen.destroy.link);
    wl_list_remove(&deco->listen.request_mode.link);
    free(deco);
}

void
bsi_decoration_draw(struct bsi_decoration* deco,
                    struct wlr_texture** texture,
                    float matrix[9],
                    struct wlr_fbox* source_box)
{
    struct bsi_server* server = deco->server;
    struct bsi_view* view = deco->view;
    struct bsi_output* output = view->parent_workspace->output;

    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, view->box.width + 5, view->box.height + 5);
    cairo_t* cr = cairo_create(surface);
    cairo_set_line_width(cr, 2.0);
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0.0, 0.0, view->box.width + 3, view->box.height + 3);
    cairo_stroke(cr);

    cairo_text_extents_t te;
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(
        cr, "SansSerif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 1.5);
    cairo_text_extents(cr, view->toplevel->app_id, &te);
    cairo_move_to(cr, view->box.width / 2.0 - te.width / 2.0, 2.0);
    cairo_show_text(cr, view->toplevel->app_id);

    uint8_t* data = cairo_image_surface_get_data(surface);
    *texture = wlr_texture_from_pixels(server->wlr_renderer,
                                       DRM_FORMAT_ARGB8888,
                                       cairo_image_surface_get_stride(surface),
                                       cairo_image_surface_get_width(surface),
                                       cairo_image_surface_get_height(surface),
                                       data);

    wlr_matrix_project_box(matrix,
                           &view->box,
                           WL_OUTPUT_TRANSFORM_NORMAL,
                           0.0f,
                           output->output->transform_matrix);

    wlr_surface_get_buffer_source_box(view->toplevel->base->surface,
                                      source_box);

    cairo_surface_destroy(surface);
    cairo_destroy(cr);
}

/**
 * Handlers
 */
void
handle_decoration_destroy(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event destroy from wlr_server_decoration");
    struct bsi_decoration* server_deco =
        wl_container_of(listener, server_deco, listen.destroy);
    bsi_decorations_remove(server_deco);
    bsi_decoration_destroy(server_deco);
}

void
handle_decoration_request_mode(struct wl_listener* listener, void* data)
{
    bsi_debug("Got event request_mode from wlr_server_decoration");
    struct bsi_decoration* server_deco =
        wl_container_of(listener, server_deco, listen.request_mode);
    struct wlr_xdg_toplevel_decoration_v1* deco = data;
    struct bsi_view* view = deco->surface->data;

    bsi_info("View with app-id '%s', requested SSD %d",
             view->toplevel->app_id,
             deco->requested_mode ==
                 WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    wlr_xdg_toplevel_decoration_v1_set_mode(deco, deco->requested_mode);
}
