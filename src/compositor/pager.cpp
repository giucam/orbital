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

namespace Orbital {

class Pager::Root : public QObject
{
public:
    class RootSurface : public DummySurface
    {
    public:
        RootSurface(Compositor *c) : DummySurface(c), view(new View(this)) { }
        View *view;
    };

    Root(Output *o, Compositor *c)
        : output(o)
        , ds(new RootSurface(c))
        , active(nullptr)
    {
        ds->view->setPos(o->pos());
    }

    ~Root()
    {
        delete ds;
    }

    Output *output;
    RootSurface *ds;
    WorkspaceView *active;
};

Pager::Pager(Compositor *c)
     : m_compositor(c)
{
    for (Output *o: c->outputs()) {
        m_roots.insert(o->id(), new Root(o, c));
    }
    connect(c, &Compositor::outputCreated, this, &Pager::outputCreated);
    connect(c, &Compositor::outputRemoved, this, &Pager::outputRemoved);
}

void Pager::addWorkspace(Workspace *ws)
{
    for (Output *o: m_compositor->outputs()) {
        WorkspaceView *wsv = ws->viewForOutput(o);
        Root *root = m_roots.value(o->id());
        wsv->setTransformParent(root->ds->view);
        wsv->setPos(ws->id(), 0);
    }
    m_workspaces << ws;
}

void Pager::activate(Workspace *ws, Output *output)
{
    WorkspaceView *wsv = ws->viewForOutput(output);
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
    if (m_workspaces.isEmpty()) {
        return;
    }

    Workspace *workspace = output->currentWorkspace();
    int index = m_workspaces.indexOf(workspace) + d;
    while (index < 0) {
        index += m_workspaces.count();
    }
    while (index >= m_workspaces.count()) {
        index -= m_workspaces.count();
    }
    activate(m_workspaces.at(index), output);
}

bool Pager::isWorkspaceActive(Workspace *ws, Output *output) const
{
    Root *root = m_roots.value(output->id());
    return root->active && root->active->workspace() == ws;
}

void Pager::activate(WorkspaceView *wsv, Output *output, bool animate)
{
    Root *root = m_roots.value(output->id());
    QPoint p = wsv->pos();
    double dx = p.x();
    double dy = p.y();

    foreach (Workspace *ws, m_workspaces) {
        WorkspaceView *wsv = ws->viewForOutput(output);
        Transform tr;
        tr.translate(-dx, -dy);
        wsv->setTransform(tr, animate);
    }

    root->active = wsv;
    Workspace *oldWs = output->m_currentWs;
    if (oldWs) {
        oldWs->m_outputs.removeOne(output);
    }

    output->m_currentWs = wsv->workspace();
    wsv->workspace()->m_outputs << output;

    if (oldWs && !oldWs->outputs().isEmpty()) {
        return;
    }

    for (Seat *seat: m_compositor->seats()) {
        seat->activate(wsv->workspace());
    }
}

void Pager::updateWorkspacesPosition(Output *output)
{
    Root *root = m_roots.value(output->id());
    activate(root->active, output, false);
}

void Pager::outputCreated(Output *o)
{
    Root *root = new Root(o, m_compositor);
    m_roots.insert(o->id(), root);
    for (Workspace *ws: m_compositor->shell()->workspaces()) {
        WorkspaceView *wsv = ws->viewForOutput(o);
        wsv->setTransformParent(root->ds->view);
        wsv->setPos(ws->id(), 0);
    }

    Workspace *ws = m_compositor->shell()->workspaces().first();
    activate(ws->viewForOutput(o), o, false);
}

void Pager::outputRemoved(Output *o)
{
    Root *root = m_roots.take(o->id());
    delete root;
}

}
