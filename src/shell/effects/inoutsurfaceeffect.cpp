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

#include "inoutsurfaceeffect.h"
#include "animation.h"
#include "shellsurface.h"

const int ALPHA_ANIM_DURATION = 200;

struct InOutSurfaceEffect::Surface {
    ShellSurface *surface;
    Animation animation;
};

InOutSurfaceEffect::InOutSurfaceEffect(Shell *shell)
                  : Effect(shell)
{
}

InOutSurfaceEffect::~InOutSurfaceEffect()
{
    for (auto i = m_surfaces.begin(); i != m_surfaces.end(); ++i) {
        delete *i;
        m_surfaces.erase(i);
    }
}

void InOutSurfaceEffect::addedSurface(ShellSurface *surface)
{
    Surface *surf = new Surface;
    surf->surface = surface;

    surf->animation.updateSignal.connect(surface, &ShellSurface::setAlpha);
    m_surfaces.push_back(surf);

    surf->animation.setStart(0);
    surf->animation.setTarget(surface->alpha());
    surf->animation.run(surface->output(), ALPHA_ANIM_DURATION);
}

void InOutSurfaceEffect::removedSurface(ShellSurface *surface)
{
    for (auto i = m_surfaces.begin(); i != m_surfaces.end(); ++i) {
        if ((*i)->surface == surface) {
            delete *i;
            m_surfaces.erase(i);
            break;
        }
    }
}
