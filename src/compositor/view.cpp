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

#include <weston-1/compositor.h>

#include "view.h"
#include "output.h"
#include "workspace.h"
#include "transform.h"
#include "layer.h"
#include "surface.h"
#include "compositor.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    View *view;
};

void View::viewDestroyed(wl_listener *listener, void *data)
{
    View *view = reinterpret_cast<Listener *>(listener)->view;
    view->m_view = nullptr;
    wl_list_remove(&listener->link);
    delete view;
}

View::View(Surface *s)
    : View(s, weston_view_create(s->surface()))
{
}

View::View(Surface *s, weston_view *view)
    : m_view(view)
    , m_surface(s)
    , m_listener(new Listener)
    , m_output(nullptr)
    , m_transform(new Transform)
    , m_pointerState({ false, nullptr })
{
    m_transform->setView(m_view);

    m_listener->listener.notify = viewDestroyed;
    m_listener->view = this;
    wl_signal_add(&m_view->destroy_signal, &m_listener->listener);

    s->m_views.push_back(this);
}

View::~View()
{
    m_surface->m_views.erase(std::find(m_surface->m_views.begin(), m_surface->m_views.end(), this));
    if (m_view) {
        wl_list_remove(&m_listener->listener.link);
        weston_view_destroy(m_view);
    }
    delete m_listener;
    delete m_transform;
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

QPointF View::pos() const
{
    return QPointF(x(), y());
}

QRectF View::geometry() const
{
    return QRectF(m_view->geometry.x, m_view->geometry.y, m_view->surface->width, m_view->surface->height);
}

double View::alpha() const
{
    return m_view->alpha;
}

void View::setOutput(Output *o)
{
    m_output = o;
    m_view->output = o->m_output;
}


void View::setAlpha(double a)
{
    m_view->alpha = a;
    weston_view_damage_below(m_view);
}

void View::setPos(double x, double y)
{
    weston_view_set_position(m_view, x, y);
    weston_view_geometry_dirty(m_view);
}

void View::setTransformParent(View *p)
{
    weston_view_set_transform_parent(m_view, p ? p->m_view : nullptr);
    weston_view_update_transform(m_view);
}

void View::setTransform(const Transform &tr)
{
    *m_transform = tr;

    weston_view_geometry_dirty(m_view);
}

const Transform &View::transform() const
{
    return *m_transform;
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

QPointF View::mapToGlobal(const QPointF &p)
{
    float x, y;
    weston_view_to_global_float(m_view, p.x(), p.y(), &x, &y);
    return QPointF(x, y);
}

View *View::mainView() const
{
    weston_view *view = m_view;
    while (view->parent_view) {
        view = view->parent_view;
    }
    return View::fromView(view);
}

void View::update()
{
    weston_view_update_transform(m_view);
}

void View::unmap()
{
    weston_view_unmap(m_view);
    Compositor *c = Compositor::fromCompositor(m_view->surface->compositor);
    c->layer(Compositor::Layer::Minimized)->addView(this);
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
    if (!m_view->output) {
        return nullptr;
    }
    return Output::fromOutput(m_view->output);
}

Surface *View::surface() const
{
    return m_surface;
}

View *View::fromView(weston_view *v)
{
    wl_listener *listener = wl_signal_get(&v->destroy_signal, viewDestroyed);
    if (!listener) {
        return new View(Surface::fromSurface(v->surface), v);
    }
    return reinterpret_cast<Listener *>(listener)->view;
}

View *View::dispatchPointerEvent(const Pointer *pointer, wl_fixed_t fx, wl_fixed_t fy)
{
    int ix = wl_fixed_to_int(fx);
    int iy = wl_fixed_to_int(fy);

    if (pixman_region32_contains_point(&m_view->transform.boundingbox, ix, iy, NULL)) {
        wl_fixed_t fvx, fvy;
        weston_view_from_global_fixed(m_view, fx, fy, &fvx, &fvy);
        if (pixman_region32_contains_point(&m_view->surface->input, wl_fixed_to_int(fvx), wl_fixed_to_int(fvy), NULL)) {
            if (m_pointerState.inside) {
                return m_pointerState.target;
            }
            m_pointerState.inside = true;
            m_pointerState.target = pointerEnter(pointer);
            return m_pointerState.target;
        }
    }

    if (m_pointerState.inside) {
        m_pointerState.inside = false;
        pointerLeave(pointer);
    }
    return nullptr;
}

}
