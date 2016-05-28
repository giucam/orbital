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

#include <compositor.h>

#include "dummysurface.h"
#include "compositor.h"

namespace Orbital {

DummySurface::DummySurface(Compositor *c, int w, int h)
            : Surface(weston_surface_create(c->m_compositor))
            , m_acceptInput(true)
{
    weston_surface *s = surface();
    weston_surface_set_color(s, 0.0, 0.0, 0.0, 1);
    setSize(w, h);
}

DummySurface::~DummySurface()
{
}

void DummySurface::setSize(int w, int h)
{
    weston_surface *s = surface();

    weston_surface_set_size(s, w, h);
    pixman_region32_fini(&s->opaque);
    pixman_region32_init_rect(&s->opaque, 0, 0, w, h);
    weston_surface_damage(s);

    if (m_acceptInput) {
        pixman_region32_fini(&s->input);
        pixman_region32_init_rect(&s->input, 0, 0, w, h);
    }
}

void DummySurface::setAcceptInput(bool accept)
{
    if (m_acceptInput == accept) {
        return;
    }

    m_acceptInput = accept;
    weston_surface *s = surface();
    int w = accept ? width() : 0;
    int h = accept ? height() : 0;
    pixman_region32_fini(&s->input);
    pixman_region32_init_rect(&s->input, 0, 0, w, h);
}

}
