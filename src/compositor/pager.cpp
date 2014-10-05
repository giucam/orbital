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
        connect(&animation, &Animation::update, this, &Root::updateAnim);
    }

    void updateAnim(double v)
    {
        QPointF pos(start * (1. - v) + end * v);
        ds->view->setPos(pos);
        QRect out = output->geometry();
        for (WorkspaceView *w: workspaces) {
            QPoint p = w->pos();
            int x = pos.x() + p.x();
            int y = pos.y() + p.y();
            w->setMask(out.intersected(QRect(x, y, out.width(), out.height())));
        }
    }

    Output *output;
    RootSurface *ds;
    QList<WorkspaceView *> workspaces;
    WorkspaceView *active;
    Animation animation;
    QPointF start, end;
};

Pager::Pager(Compositor *c)
     : m_compositor(c)
{
    for (Output *o: c->outputs()) {
        m_roots.insert(o->id(), new Root(o, c));
    }
    connect(c, &Compositor::outputCreated, this, &Pager::outputCreated);
}

void Pager::addWorkspace(Workspace *ws)
{
    for (Output *o: m_compositor->outputs()) {
        WorkspaceView *wsv = ws->viewForOutput(o);
        Root *root = m_roots.value(o->id());
        root->workspaces << wsv;
        wsv->setTransformParent(root->ds->view);
        wsv->setPos(ws->id() * o->width(), 0);
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

    root->start = root->ds->view->pos();
    root->end = QPointF(output->x() - dx, output->y() - dy);
    if (animate) {
        root->animation.setStart(0);
        root->animation.setTarget(1);
        root->animation.run(output, 200);
    } else {
        root->updateAnim(1);
    }

    root->active = wsv;
    output->m_currentWs = wsv->workspace();
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
        root->workspaces << wsv;
        wsv->setTransformParent(root->ds->view);
        wsv->setPos(ws->id() * o->width(), 0);
    }

    Workspace *ws = m_compositor->shell()->workspaces().first();
    activate(ws->viewForOutput(o), o, false);
}

}
