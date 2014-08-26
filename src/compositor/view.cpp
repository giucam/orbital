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

#include <weston/compositor.h>

#include "view.h"
#include "output.h"
#include "workspace.h"
#include "transform.h"
#include "layer.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    View *view;
};

static void viewDestroyed(wl_listener *listener, void *data)
{
}

View::View(weston_view *view)
    : m_view(view)
    , m_listener(new Listener)
    , m_output(nullptr)
    , m_transform(nullptr)
{
    m_listener->listener.notify = viewDestroyed;
    m_listener->view = this;
    wl_signal_add(&view->destroy_signal, &m_listener->listener);
}

View::~View()
{
    weston_view_destroy(m_view);
}

bool View::isMapped() const
{
    return weston_view_is_mapped(m_view);
}

double View::x() const
{
    return m_view->geometry.x;
}

double View::y() const
{
    return m_view->geometry.y;
}

QRectF View::geometry() const
{
    return QRectF(m_view->geometry.x, m_view->geometry.y, m_view->surface->width, m_view->surface->height);
}

void View::setOutput(Output *o)
{
    m_output = o;
    m_view->output = o->m_output;
}


void View::setAlpha(double a)
{
    m_view->alpha = a;
}

void View::setPos(double x, double y)
{
    weston_view_set_position(m_view, x, y);
    weston_view_geometry_dirty(m_view);
}

void View::setTransformParent(View *p)
{
    weston_view_set_transform_parent(m_view, p->m_view);
    weston_view_update_transform(m_view);
}

void View::setTransform(const Transform &tr)
{
    if (!m_transform) {
        m_transform = new Transform;
        m_transform->setView(m_view);
    }
    *m_transform = tr;
}

QPointF View::mapFromGlobal(const QPointF &p)
{
    wl_fixed_t x, y;
    wl_fixed_t vx, vy;

    x = wl_fixed_from_double(p.x());
    y = wl_fixed_from_double(p.y());

    weston_view_from_global_fixed(m_view, x, y, &vx, &vy);

    return QPointF(wl_fixed_to_double(vx), wl_fixed_to_double(vy));
}

void View::update()
{
    weston_view_update_transform(m_view);
}

void View::unmap()
{
    weston_layer_entry_remove(&m_view->layer_link);
}

wl_client *View::client() const
{
    return m_view->surface->resource ? wl_resource_get_client(m_view->surface->resource) : nullptr;
}

Layer *View::layer() const
{
    weston_layer *layer = m_view->layer_link.layer;
    if (layer) {
        return Layer::fromLayer(layer);
    }
    return nullptr;
}

Output *View::output() const
{
    return m_output;
}

weston_surface *View::surface() const
{
    return m_view->surface;
}

View *View::fromView(weston_view *v)
{
    wl_listener *listener = wl_signal_get(&v->destroy_signal, viewDestroyed);
    Q_ASSERT(listener);
    return reinterpret_cast<Listener *>(listener)->view;
}

}
