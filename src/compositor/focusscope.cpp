/*
 * Copyright 2014-2015 Giulio Camuffo <giuliocamuffo@gmail.com>
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


#include "focusscope.h"
#include "surface.h"
#include "seat.h"
#include "workspace.h"
#include "shell.h"
#include "compositor.h"
#include "output.h"
#include "debug.h"

namespace Orbital {

FocusScope::FocusScope(Shell *shell)
          : QObject(shell)
          , m_shell(shell)
          , m_activeSurface(nullptr)
{
}

FocusScope::~FocusScope()
{
    if (m_activeSurface) {
        for (Seat *seat: m_activeSeats) {
            emit m_activeSurface->deactivated(seat);
        }
    }
}

Surface *FocusScope::activate(Surface *surface)
{
    Debug::debug("Requested to activate surface {}, isActivable: {}. Current active is {}", surface, (surface ? surface->isActivable() : false), m_activeSurface);
    bool isNull = !surface;
    if (surface) {
        surface = surface->isActivable() ? surface : nullptr;
    }

    if (surface) {
        if (surface->focusScope() != this && surface->focusScope()) {
            qWarning("Tried to activate surface %p on wrong FocusScope. This: %p, surface has %p", surface, this, surface->focusScope());
            return nullptr;
        }
        surface->setFocusScope(this);
    }

    if ((!surface && !isNull) || surface == m_activeSurface) {
        return m_activeSurface;
    }

    if (surface || isNull) {
        for (Seat *seat: m_activeSeats) {
            weston_seat_set_keyboard_focus(seat->m_seat, surface ? surface->surface() : nullptr);
        }
    }

    if (m_activeSurface) {
        for (Seat *seat: m_activeSeats) {
            emit m_activeSurface->deactivated(seat);
        }
    }
    m_activeSurface = surface;
    Debug::debug("Activate surface {}", surface);
    if (m_activeSurface) {
        for (Seat *seat: m_activeSeats) {
            emit m_activeSurface->activated(seat);
        }
        connect(m_activeSurface, &Surface::unmapped, this, &FocusScope::deactivateSurface);

        m_activeSurfaces.remove(surface);
        m_activeSurfaces.push_front(surface);
    }
    return m_activeSurface;
}

void FocusScope::activate(Workspace *ws)
{
    Debug::debug("Activate workspace {}, id {}. Active surfaces are: [", (void *)ws, ws->id());
    Surface *surface = nullptr;
    for (Surface *s: m_activeSurfaces) {
        Debug::debug("\t{}", s);
        if (s->workspaceMask() & ws->mask() || s->workspaceMask() == -1) {
            surface = s;
            break;
        }
    }
    Debug::debug("]");
    activate(surface);
}

void FocusScope::deactivateSurface()
{
    Surface *surface = static_cast<Surface *>(sender());

    Debug::debug("Surface {} deactivated", surface);

    m_activeSurfaces.remove(surface);
    if (surface == m_activeSurface) {
        for (Seat *seat: m_activeSeats) {
            m_activeSurface->deactivated(seat);
        }
        m_activeSurface = nullptr;

        if (m_activeSurfaces.empty()) {
            return;
        }

        int mask = 0;
        for (Output *out: m_shell->compositor()->outputs()) {
            mask |= out->currentWorkspace()->mask();
        }
        for (Surface *surf: m_activeSurfaces) {
            if (surf->workspaceMask() & mask) {
                activate(surf);
                break;
            }
        }
    }
}

void FocusScope::activated(Seat *s)
{
    if (std::find(m_activeSeats.begin(), m_activeSeats.end(), s) != m_activeSeats.end()) {
        return;
    }
    m_activeSeats.push_back(s);
}

void FocusScope::deactivated(Seat *s)
{
    m_activeSeats.remove(s);
}

}
