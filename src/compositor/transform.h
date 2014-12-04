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

#ifndef ORBITAL_TRANSFORM_H
#define ORBITAL_TRANSFORM_H

#include <compositor.h>

namespace Orbital {

class View;

class Transform {
public:
    Transform();
    Transform(const Transform &t);

    void reset();
    void scale(double x, double y);
    void translate(double x, double y);

    Transform &operator=(const Transform &t);

private:
    void setView(weston_view *v);

    weston_transform m_transform;
    weston_view *m_view;

    friend View;
};

}

#endif
