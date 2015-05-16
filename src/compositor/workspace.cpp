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
#include "transform.h"

namespace Orbital {

class Root : public DummySurface
{
public:
    Root(Compositor *c) : DummySurface(c), view(new View(this)) { }
    View *view;
};

Workspace::Workspace(Shell *shell, int id)
         : Object(shell)
         , m_shell(shell)
         , m_id(id)
         , m_x(0)
         , m_y(0)
{
    connect(shell->compositor(), &Compositor::outputRemoved, this, &Workspace::outputRemoved);
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
        WorkspaceView *view = new WorkspaceView(this, o);
        m_views.insert(o->id(), view);
        view->m_root->view->setPos(m_x * o->width(), m_y * o->height());
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

void Workspace::setPos(int x, int y)
{
    if (m_x == x && m_y == y) {
        return;
    }

    m_x = x;
    m_y = y;

    foreach (WorkspaceView *v, m_views) {
        v->m_root->view->setPos(x * v->m_output->width(), y * v->m_output->height());
    }
    emit positionChanged(x, y);
}

int Workspace::x() const
{
    return m_x;
}

int Workspace::y() const
{
    return m_y;
}

void Workspace::outputRemoved(Output *o)
{
    delete m_views.take(o->id());
}


WorkspaceView::WorkspaceView(Workspace *ws, Output *o)
             : QObject()
             , m_workspace(ws)
             , m_output(o)
             , m_backgroundLayer(new Layer(ws->compositor()->backgroundLayer()))
             , m_layer(new Layer(ws->compositor()->appsLayer()))
             , m_fullscreenLayer(new Layer(ws->compositor()->fullscreenLayer()))
             , m_background(nullptr)
{
    m_root = new Root(ws->compositor());

    connect(&m_transformAnim.anim, &Animation::update, this, &WorkspaceView::updateAnim);

    resetMask();
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

QPoint WorkspaceView::logicalPos() const
{
    return QPoint(m_root->view->x() / m_output->width(), m_root->view->y() / m_output->height());
}

QPointF WorkspaceView::map(double x, double y) const
{
    return m_root->view->mapFromGlobal(QPointF(x, y));
}

bool WorkspaceView::ownsView(View *view) const
{
    Layer *l = view->layer();
    return l == m_layer || l == m_backgroundLayer || l == m_fullscreenLayer;
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

void WorkspaceView::resetMask()
{
    m_root->view->update();
    QPointF tl = m_root->view->mapToGlobal(QPointF(0, 0));
    QPointF br = m_root->view->mapToGlobal(QPointF(m_output->width(), m_output->height()));
    setMask(QRect(QPoint(qRound(tl.x()), qRound(tl.y())), QPoint(qRound(br.x() - 1), qRound(br.y() - 1))));
}

void WorkspaceView::setMask(const QRect &m)
{
    QRect r = m_output->geometry().intersected(m);
    m_backgroundLayer->setMask(r.x(), r.y(), r.width(), r.height());
    m_layer->setMask(r.x(), r.y(), r.width(), r.height());
    m_fullscreenLayer->setMask(r.x(), r.y(), r.width(), r.height());
    m_mask = r;
}

void WorkspaceView::setTransform(const Transform &tf, bool animate)
{
    m_transformAnim.target = tf;
    if (animate) {
        m_transformAnim.orig = transform();
        m_transformAnim.anim.run(m_output, 300);
    } else {
        updateAnim(1.);
    }
}

void WorkspaceView::updateAnim(double v)
{
    m_root->view->setTransform(Transform::interpolate(m_transformAnim.orig, m_transformAnim.target, v));
    resetMask();
    m_output->repaint();
}

const Transform &WorkspaceView::transform() const
{
    return m_root->view->transform();
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
