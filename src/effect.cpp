/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "effect.h"
#include "shell.h"

Effect::Effect(Shell *shell)
      : m_shell(shell)
{

}

void Effect::addSurface(ShellSurface *surf)
{
    SurfaceTransform tr;
    tr.surface = surf;

    m_surfaces.push_back(tr);
    wl_list_init(&m_surfaces.back().transform.link);

    addedSurface(surf);
}

const struct weston_layer *Effect::layer() const
{
    return m_shell->layer();
}

std::vector<Effect::SurfaceTransform> &Effect::surfaces()
{
    return m_surfaces;
}
