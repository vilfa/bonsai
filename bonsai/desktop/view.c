#include <stdlib.h>
#include <wayland-util.h>

#include "bonsai/desktop/view.h"

struct bsi_view*
view_init(struct bsi_view* view,
          enum bsi_view_type type,
          const struct bsi_view_impl* impl,
          struct bsi_server* server)
{
    view->type = type;
    view->impl = impl;
    view->server = server;
    view->workspace = NULL;
    view->mapped = false;
    view->tree = NULL;
    view->inhibit.fullscreen = NULL;
    view->state = BSI_VIEW_STATE_NORMAL;
    return view;
}

void
view_destroy(struct bsi_view* view)
{
    if (view->impl->destroy) {
        view->impl->destroy(view);
    } else {
        free(view);
    }
}

void
view_focus(struct bsi_view* view)
{
    view->impl->focus(view);
}

void
view_cursor_interactive(struct bsi_view* view,
                        enum bsi_cursor_mode cursor_mode,
                        union bsi_xdg_toplevel_event toplevel_event)
{
    view->impl->cursor_interactive(view, cursor_mode, toplevel_event);
}

void
view_set_maximized(struct bsi_view* view, bool maximized)
{
    view->impl->set_maximized(view, maximized);
}

void
view_set_minimized(struct bsi_view* view, bool minimized)
{
    view->impl->set_minimized(view, minimized);
}

void
view_set_fullscreen(struct bsi_view* view, bool fullscreen)
{
    view->impl->set_fullscreen(view, fullscreen);
}

void
view_set_tiled_left(struct bsi_view* view, bool tiled)
{
    view->impl->set_tiled_left(view, tiled);
}

void
view_set_tiled_right(struct bsi_view* view, bool tiled)
{
    view->impl->set_tiled_right(view, tiled);
}

void
view_restore_prev(struct bsi_view* view)
{
    view->impl->restore_prev(view);
}

bool
view_intersects(struct bsi_view* view, struct wlr_box* box)
{
    return view->impl->intersects(view, box);
}

void
view_get_correct(struct bsi_view* view,
                 struct wlr_box* box,
                 struct wlr_box* correction)
{
    view->impl->get_correct(view, box, correction);
}

void
view_set_correct(struct bsi_view* view, struct wlr_box* correction)
{
    view->impl->set_correct(view, correction);
}

void
view_request_activate(struct bsi_view* view)
{
    view->impl->request_activate(view);
}
