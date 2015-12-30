/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_PAGER_H
#define ORBITAL_PAGER_H

#include <unordered_map>
#include <vector>

#include <QObject>

#include "workspace.h"

namespace Orbital {

class Compositor;
class DummySurface;
class Output;

class Pager : public QObject
{
    Q_OBJECT
public:
    Pager(Compositor *c);

    void addWorkspace(Workspace *ws);
    void activate(Workspace *ws, Output *output);
    void activateNextWorkspace(Output *output);
    void activatePrevWorkspace(Output *output);
    bool isWorkspaceActive(AbstractWorkspace *ws, Output *output) const;
    void updateWorkspacesPosition(Output *o);

signals:
    void workspaceActivated(Workspace *ws, Output *o);

private:
    void activate(Workspace::View *wsv, Output *o, bool animate);
    void outputCreated(Output *o);
    void outputRemoved(Output *o);
    void changeWorkspace(Output *o, int d);

    class Root;

    Compositor *m_compositor;
    std::unordered_map<int, Root *> m_roots;
    std::vector<Workspace *> m_workspaces;
};

}

#endif
