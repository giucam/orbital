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

#include <QDebug>

#include <compositor.h>

#include "workspace.h"
#include "shell.h"
#include "layer.h"
#include "view.h"
#include "shellsurface.h"
#include "output.h"
#include "compositor.h"
#include "dummysurface.h"
#include "shellview.h"
#include "pager.h"

namespace Orbital {

Workspace::Workspace(Shell *shell, int id)
         : Object(shell)
         , m_shell(shell)
         , m_id(id)
{

}

Workspace::~Workspace()
{
    for (WorkspaceView *wsv: m_views) {
        delete wsv;
    }
}

Compositor *Workspace::compositor() const
{
    return m_shell->compositor();
}

Pager *Workspace::pager() const
{
    return m_shell->pager();
}

WorkspaceView *Workspace::viewForOutput(Output *o)
{
    if (!m_views.contains(o->id())) {
        WorkspaceView *view = new WorkspaceView(this, o, o->width(), o->height());
        m_views.insert(o->id(), view);
        return view;
    }

    return m_views.value(o->id());
}

View *Workspace::topView() const
{
    WorkspaceView *view = *m_views.begin();
    return view->m_layer->topView();
}

int Workspace::id() const
{
    return m_id;
}

int Workspace::mask() const
{
    return 1 << m_id;
}


class Root : public DummySurface
{
public:
    Root(Compositor *c) : DummySurface(c), view(new View(this)) { }
    View *view;
};

WorkspaceView::WorkspaceView(Workspace *ws, Output *o, int w, int h)
             : m_workspace(ws)
             , m_output(o)
             , m_width(w)
             , m_height(h)
             , m_backgroundLayer(new Layer)
             , m_layer(new Layer)
             , m_fullscreenLayer(new Layer)
             , m_background(nullptr)
{
    m_root = new Root(ws->compositor());

    m_backgroundLayer->append(ws->compositor()->backgroundLayer());
    m_layer->append(ws->compositor()->appsLayer());
    m_fullscreenLayer->append(ws->compositor()->fullscreenLayer());

    setMask(QRect());
}

WorkspaceView::~WorkspaceView()
{
    delete m_root;
    delete m_background;
    delete m_backgroundLayer;
    delete m_layer;
    delete m_fullscreenLayer;
}

QPoint WorkspaceView::pos() const
{
    return QPoint(m_root->view->x(), m_root->view->y());
}

void WorkspaceView::setPos(int x, int y)
{
    m_root->view->setPos(x, y);
}

void WorkspaceView::setTransformParent(View *p)
{
    m_root->view->setTransformParent(p);
}

void WorkspaceView::setBackground(Surface *s)
{
    if (m_background && m_background->surface() == s) {
        return;
    }

    if (m_background) {
        m_background->surface()->deref();
    }
    // increase the ref count for the background, so to keep it alive when the shell client
    // crashes, until a new one is set
    s->ref();
    m_background = new View(s);
    m_background->setTransformParent(m_root->view);
    m_backgroundLayer->addView(m_background);
}

void WorkspaceView::setMask(const QRect &r)
{
    m_backgroundLayer->setMask(r.x(), r.y(), r.width(), r.height());
    m_layer->setMask(r.x(), r.y(), r.width(), r.height());
    m_fullscreenLayer->setMask(r.x(), r.y(), r.width(), r.height());
}

void WorkspaceView::configure(View *view)
{
    if (view->layer() != m_layer) {
        m_layer->addView(view);
        view->setTransformParent(m_root->view);
    }
}

void WorkspaceView::configureFullscreen(View *view, View *blackSurface)
{
    m_fullscreenLayer->addView(blackSurface);
    m_fullscreenLayer->addView(view);
    view->setTransformParent(m_root->view);
    blackSurface->setTransformParent(m_root->view);
}

}
