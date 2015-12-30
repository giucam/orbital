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

#include <QDebug>

#include "pager.h"
#include "compositor.h"
#include "workspace.h"
#include "output.h"
#include "dummysurface.h"
#include "animation.h"
#include "shell.h"
#include "seat.h"
#include "transform.h"
#include "focusscope.h"

namespace Orbital {

class Pager::Root : public QObject
{
public:
    Root(Output *o, Compositor *c)
        : output(o)
        , active(nullptr)
    {
    }

    ~Root()
    {
    }

    Output *output;
    Workspace::View *active;
};

Pager::Pager(Compositor *c)
     : m_compositor(c)
{
    for (Output *o: c->outputs()) {
        m_roots[o->id()] = new Root(o, c);
    }
    connect(c, &Compositor::outputCreated, this, &Pager::outputCreated);
    connect(c, &Compositor::outputRemoved, this, &Pager::outputRemoved);
}

void Pager::addWorkspace(Workspace *ws)
{
    m_workspaces.push_back(ws);

    int n = ws->id();
    int rows = n > 2 ? 2 : 1;
    int cols = ceil((double)(n + 1) / (double)rows);
    int x = 0, y = 0;
    for (Workspace *w: m_workspaces) {
        w->setPos(x, y);
        if (++x >= cols) {
            x = 0;
            ++y;
        }
    }
}

void Pager::activate(Workspace *ws, Output *output)
{
    Workspace::View *wsv = workspaceViewForOutput(ws, output);
    activate(wsv, output, true);
    emit workspaceActivated(wsv->workspace(), output);
}

void Pager::activateNextWorkspace(Output *output)
{
    changeWorkspace(output, 1);
}

void Pager::activatePrevWorkspace(Output *output)
{
    changeWorkspace(output, -1);
}

void Pager::changeWorkspace(Output *output, int d)
{
    if (m_workspaces.empty()) {
        return;
    }

    Workspace *workspace = output->currentWorkspace();
    int index = d;
    int size = m_workspaces.size();
    for (int i = 0; i < size; ++i) {
        if (m_workspaces[i] == workspace) {
            index += i;
            break;
        }
    }
    while (index < 0) {
        index += size;
    }
    while (index >= size) {
        index -= size;
    }
    activate(m_workspaces[index], output);
}

bool Pager::isWorkspaceActive(AbstractWorkspace *ws, Output *output) const
{
    const Root *root = m_roots.at(output->id());
    return root->active && root->active->workspace() == ws;
}

void Pager::activate(Workspace::View *wsv, Output *output, bool animate)
{
    Root *root = m_roots[output->id()];
    QPoint p = wsv->pos();
    double dx = p.x();
    double dy = p.y();

    for (Workspace *ws: m_workspaces) {
        Workspace::View *wsv = workspaceViewForOutput(ws, output);
        Transform tr;
        tr.translate(-dx, -dy);
        wsv->setTransform(tr, animate);
    }

    root->active = wsv;
    output->m_currentWs = wsv->workspace();

    m_compositor->shell()->appsFocusScope()->activate(wsv->workspace());
}

void Pager::updateWorkspacesPosition(Output *output)
{
    Root *root = m_roots[output->id()];
    activate(root->active, output, false);
}

void Pager::outputCreated(Output *o)
{
    Root *root = new Root(o, m_compositor);
    m_roots[o->id()] = root;

    Workspace *ws = m_compositor->shell()->workspaces().front();
    activate(workspaceViewForOutput(ws, o), o, false);
    emit workspaceActivated(ws, o);
}

void Pager::outputRemoved(Output *o)
{
    Root *root = m_roots[o->id()];
    m_roots.erase(o->id());
    delete root;
}

}
