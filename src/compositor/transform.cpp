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

#include <matrix.h>

#include "transform.h"

namespace Orbital {

Transform::Transform()
         : m_view(nullptr)
{
    wl_list_init(&m_transform.link);
    reset();
}

Transform::Transform(const Transform &t)
         : Transform()
{
    *this = t;
}

void Transform::reset()
{
    weston_matrix_init(&m_transform.matrix);
}

void Transform::scale(double x, double y)
{
    weston_matrix_scale(&m_transform.matrix, x, y, 1);
}

void Transform::translate(double x, double y)
{
    weston_matrix_translate(&m_transform.matrix, x, y, 0);
}

Transform &Transform::operator=(const Transform &t)
{
    wl_list_remove(&m_transform.link);
    wl_list_init(&m_transform.link);
    m_transform = t.m_transform;
    wl_list_init(&m_transform.link);

    if (m_view) {
        wl_list_insert(&m_view->geometry.transformation_list, &m_transform.link);
    }

    return *this;
}

void Transform::setView(weston_view *v)
{
    wl_list_remove(&m_transform.link);
    wl_list_init(&m_transform.link);
    m_view = v;

    if (m_view) {
        wl_list_insert(&m_view->geometry.transformation_list, &m_transform.link);
    }
}

}