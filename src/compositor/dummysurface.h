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

#ifndef ORBITAL_DUMMYSURFACE_H
#define ORBITAL_DUMMYSURFACE_H

#include "view.h"
#include "surface.h"

struct weston_surface;

namespace Orbital {

class Compositor;

class DummySurface : public Surface
{
public:
    explicit DummySurface(Compositor *c, int width = 0, int height = 0);
    ~DummySurface();

    void setSize(int width, int height);
    void setAcceptInput(bool m_acceptInput);

private:
    DummySurface(weston_surface *s, int width, int height);

    bool m_acceptInput;

    friend Compositor;
};

}

#endif
