
#include <weston/compositor.h>

#include "dummysurface.h"

namespace Orbital {

DummySurface::DummySurface(weston_surface *s, int w, int h)
            : View(weston_view_create(s))
            , m_surface(s)
{
    s->configure = [](struct weston_surface *es, int32_t sx, int32_t sy) {};
    s->configure_private = nullptr;
    s->width = w;
    s->height = h;
    weston_surface_set_color(s, 0.0, 0.0, 0.0, 1);
    pixman_region32_fini(&s->opaque);
    pixman_region32_init_rect(&s->opaque, 0, 0, w, h);
    pixman_region32_fini(&s->input);
    pixman_region32_init_rect(&s->input, 0, 0, w, h);
    weston_surface_damage(s);
}

DummySurface::~DummySurface()
{
    weston_surface_destroy(m_surface);
}

}
