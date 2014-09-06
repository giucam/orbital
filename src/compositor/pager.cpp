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

namespace Orbital {

class Pager::Root : public QObject
{
public:
    Root(Output *o, DummySurface *d)
        : output(o)
        , ds(d)
        , active(nullptr)
    {
        connect(&animation, &Animation::update, this, &Root::updateAnim);
    }

    void updateAnim(double v)
    {
        QPointF pos(start * (1. - v) + end * v);
        ds->setPos(pos);
        QRect out = output->geometry();
        for (WorkspaceView *w: workspaces) {
            int x = pos.x() + w->m_root->x();
            int y = pos.y() + w->m_root->y();
            w->setMask(out.intersected(QRect(x, y, out.width(), out.height())));
        }
    }

    Output *output;
    DummySurface *ds;
    QList<WorkspaceView *> workspaces;
    WorkspaceView *active;
    Animation animation;
    QPointF start, end;
};

Pager::Pager(Compositor *c)
     : m_compositor(c)
{
    for (Output *o: c->outputs()) {
        DummySurface *ds = c->createDummySurface(0, 0);
        m_roots.insert(o->id(), new Root(o, ds));
    }
}

void Pager::addWorkspace(Workspace *ws)
{
    for (Output *o: m_compositor->outputs()) {
        WorkspaceView *wsv = ws->viewForOutput(o);
        Root *root = m_roots.value(o->id());
        root->workspaces << wsv;
        wsv->m_root->setTransformParent(root->ds);
        wsv->m_root->setPos(ws->id() * o->width(), 0);
    }
}

void Pager::activate(Workspace *ws, Output *output)
{
    WorkspaceView *wsv = ws->viewForOutput(output);
    activate(wsv, output, true);
    emit workspaceActivated(wsv->workspace(), output);
}

bool Pager::isWorkspaceActive(Workspace *ws, Output *output) const
{
    Root *root = m_roots.value(output->id());
    return root->active && root->active->workspace() == ws;
}

void Pager::activate(WorkspaceView *wsv, Output *output, bool animate)
{
    Root *root = m_roots.value(output->id());
    double dx = wsv->m_root->x();
    double dy = wsv->m_root->y();

    root->start = root->ds->pos();
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

}
