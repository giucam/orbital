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

#ifndef ORBITAL_SHELL_H
#define ORBITAL_SHELL_H

#include <functional>

#include "interface.h"

namespace Orbital {

class Compositor;
class Layer;
class Workspace;
class ShellSurface;
class Pointer;
class Binding;
class Seat;
enum class PointerCursor: unsigned int;

class Shell : public Object
{
    Q_OBJECT
public:
    typedef std::function<void (Pointer *, PointerCursor)> GrabCursorSetter;

    explicit Shell(Compositor *c);

    Compositor *compositor() const;
    void addWorkspace(Workspace *ws);
    QList<Workspace *> workspaces() const;

    void setGrabCursor(Pointer *pointer, PointerCursor c);
    void configure(ShellSurface *shsurf);

    void setGrabCursorSetter(GrabCursorSetter s);

private:
    void giveFocus(Seat *s);

    Compositor *m_compositor;
    QList<Workspace *> m_workspaces;
    GrabCursorSetter m_grabCursorSetter;
    Binding *m_focusBinding;
};

}

#endif
