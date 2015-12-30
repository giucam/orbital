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


#ifndef ORBITAL_FOCUSSCOPE_H
#define ORBITAL_FOCUSSCOPE_H

#include <list>

#include <QObject>

namespace Orbital {

class Shell;
class Workspace;
class Surface;
class Seat;

class FocusScope : public QObject
{
public:
    FocusScope(Shell *shell);
    ~FocusScope();

    void activate(Workspace *ws);
    Surface *activate(Surface *surface);
    Surface *activeSurface() const { return m_activeSurface; }

private:
    void deactivateSurface();
    void activated(Seat *seat);
    void deactivated(Seat *seat);

    Shell *m_shell;
    std::list<Seat *> m_activeSeats;
    std::list<Surface *> m_activeSurfaces;
    Surface *m_activeSurface;

    friend Seat;
};

}

#endif
