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

AbstractWorkspace::View::View(Compositor *c, Output *o)
                 : m_root(new Root(c))
                 , m_output(o)
{
    QObject::connect(&m_transformAnim.anim, &Animation::update, [this](float v) { updateAnim(v); });
    resetMask();
}

AbstractWorkspace::View::~View()
{
    delete m_root;
}

QPointF AbstractWorkspace::View::map(double x, double y) const
{
    return m_root->view->mapFromGlobal(QPointF(x, y));
}

QPoint AbstractWorkspace::View::pos() const
{
    return QPoint(m_root->view->x(), m_root->view->y());
}

void AbstractWorkspace::View::setPos(double x, double y)
{
    m_root->view->setPos(x, y);
}

void AbstractWorkspace::View::setTransformParent(Orbital::View *p)
{
    m_root->view->setTransformParent(p);
}

void AbstractWorkspace::View::takeView(Orbital::View *p)
{
    p->setTransformParent(m_root->view);
}

void AbstractWorkspace::View::resetMask()
{
    m_root->view->update();
    QPointF tl = m_root->view->mapToGlobal(QPointF(0, 0));
    QPointF br = m_root->view->mapToGlobal(QPointF(m_output->width(), m_output->height()));
    QRect mask(QRect(QPoint(qRound(tl.x()), qRound(tl.y())), QPoint(qRound(br.x() - 1), qRound(br.y() - 1))));
    m_mask = mask;
    setMask(m_output->geometry().intersected(mask));
}

void AbstractWorkspace::View::setTransform(const Transform &tf, bool animate)
{
    m_transformAnim.target = tf;
    if (animate) {
        m_transformAnim.orig = transform();
        m_transformAnim.anim.run(m_output, 300);
    } else {
        updateAnim(1.);
    }
}

void AbstractWorkspace::View::updateAnim(double v)
{
    m_root->view->setTransform(Transform::interpolate(m_transformAnim.orig, m_transformAnim.target, v));
    resetMask();
    m_output->repaint();
}

const Transform &AbstractWorkspace::View::transform() const
{
    return m_root->view->transform();
}


Workspace::Workspace(Shell *shell, int id)
         : Object(shell)
         , m_shell(shell)
         , m_id(id)
         , m_x(0)
         , m_y(0)
{
    setMask(1 << m_id);
    connect(shell->compositor(), &Compositor::outputRemoved, this, &Workspace::outputRemoved);
    connect(shell->compositor(), &Compositor::outputCreated, this, &Workspace::newOutput);

    for (Output *o: shell->compositor()->outputs()) {
        newOutput(o);
    }
}

Workspace::~Workspace()
{
    for (View *wsv: m_views) {
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

void Workspace::newOutput(Output *o)
{
    // make sure to create the workspace view immediately so that the layers order is right
    viewForOutput(o);
}

AbstractWorkspace::View *Workspace::viewForOutput(Output *o)
{
    if (!m_views.contains(o->id())) {
        View *view = new View(this, o);
        m_views.insert(o->id(), view);
        view->setTransformParent(o->rootView());
        view->setPos(m_x * o->width(), m_y * o->height());
        return view;
    }

    return m_views.value(o->id());
}

void Workspace::activate(Output *o)
{
    m_shell->pager()->activate(this, o);
}

Orbital::View *Workspace::topView() const
{
    View *view = *m_views.begin();
    return view->m_layer->topView();
}

int Workspace::id() const
{
    return m_id;
}

void Workspace::setPos(int x, int y)
{
    if (m_x == x && m_y == y) {
        return;
    }

    m_x = x;
    m_y = y;

    foreach (View *v, m_views) {
        v->setPos(x * v->m_output->width(), y * v->m_output->height());
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


Workspace::View::View(Workspace *ws, Output *o)
               : AbstractWorkspace::View(ws->compositor(), o)
               , m_workspace(ws)
               , m_output(o)
               , m_backgroundLayer(new Layer(ws->compositor()->layer(Compositor::Layer::Background)))
               , m_layer(new Layer(ws->compositor()->layer(Compositor::Layer::Apps)))
               , m_fullscreenLayer(new Layer(ws->compositor()->layer(Compositor::Layer::Fullscreen)))
               , m_background(nullptr)
{
}

Workspace::View::~View()
{
    delete m_background;
    delete m_backgroundLayer;
    delete m_layer;
    delete m_fullscreenLayer;
}

QPoint Workspace::View::logicalPos() const
{
    return QPoint(pos().x() / m_output->width(), pos().y() / m_output->height());
}

bool Workspace::View::ownsView(Orbital::View *view) const
{
    Layer *l = view->layer();
    return l == m_layer || l == m_backgroundLayer || l == m_fullscreenLayer;
}

void Workspace::View::setBackground(Surface *s)
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
    m_background = new Orbital::View(s);
    takeView(m_background);
    m_backgroundLayer->addView(m_background);
}

void Workspace::View::setMask(const QRect &r)
{
    m_backgroundLayer->setMask(r.x(), r.y(), r.width(), r.height());
    m_layer->setMask(r.x(), r.y(), r.width(), r.height());
    m_fullscreenLayer->setMask(r.x(), r.y(), r.width(), r.height());
}

void Workspace::View::configure(Orbital::View *view)
{
    if (view->layer() != m_layer) {
        m_layer->addView(view);
        takeView(view);
    }
}

void Workspace::View::configureFullscreen(Orbital::View *view, Orbital::View *blackSurface)
{
    m_fullscreenLayer->addView(blackSurface);
    m_fullscreenLayer->addView(view);
    takeView(view);
    takeView(blackSurface);
}

}
