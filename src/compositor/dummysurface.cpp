/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <weston/compositor.h>

#include "dummysurface.h"
#include "compositor.h"

namespace Orbital {

DummySurface::DummySurface(weston_surface *s, int w, int h)
            : Surface(s)
            , View(this)
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

DummySurface::DummySurface(Compositor *c, int w, int h)
            : DummySurface(weston_surface_create(c->m_compositor), w, h)
{
}

DummySurface::~DummySurface()
{
    disconnectDestroyListener();
    weston_surface_destroy(m_surface);
}

}
