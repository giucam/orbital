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

#ifndef ORBITAL_DESKTOPGRID_H
#define ORBITAL_DESKTOPGRID_H

#include "../effect.h"

namespace Orbital {

class Shell;
class Seat;
class Pointer;
class AxisBinding;
enum class KeyboardModifiers : unsigned char;

class DesktopGrid : public Effect
{
public:
    explicit DesktopGrid(Shell *shell);
    ~DesktopGrid();

private:
    class Grab;

    void runKey(Seat *seat, uint32_t time, int key);
    void runHotSpot(Seat *seat, uint32_t time, PointerHotSpot hs);
    void run(Seat *seat);
    void terminate(Output *out, Workspace *ws);
    void outputCreated(Output *o);
    void outputRemoved(Output *o);
    void pointerEnter(Pointer *p);
    void workspaceActivated(Workspace *ws, Output *out);

    Shell *m_shell;
    HotSpotBinding *m_hsBinding;
    QSet<Output *> m_activeOutputs;
};

}

#endif
