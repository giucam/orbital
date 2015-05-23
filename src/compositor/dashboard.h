/*
 * Copyright 2015  Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_DASHBOARD_H
#define ORBITAL_DASHBOARD_H

#include <QObject>

#include "workspace.h"

namespace Orbital {

class Shell;
class Seat;
class ButtonBinding;
class HotSpotBinding;

class Dashboard : public QObject, public AbstractWorkspace
{
public:
    class View;

    Dashboard(Shell *shell);

    AbstractWorkspace::View *viewForOutput(Output *o) override;
    void activate(Output *o) override;

private:
    void toggleSurface(Seat *seat);
    void toggle(Seat *seat);

    Shell *m_shell;
    QHash<int, View *> m_views;
    bool m_visible;
    ButtonBinding *m_toggleSurfaceBinding;
    HotSpotBinding *m_toggleBinding;
};

}

#endif
