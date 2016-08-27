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

#ifndef ORBITAL_VIEW_H
#define ORBITAL_VIEW_H

#include <QObject>
#include <QRectF>

#include <wayland-server.h>

struct wl_client;
struct wl_listener;
struct weston_view;

namespace Orbital
{

class Output;
class Layer;
class Pointer;
class Transform;
class Surface;
struct Listener;

class View;

class ViewCreator
{
public:
    virtual weston_view *create(Surface *surface) = 0;
    virtual void destroy(View *view, weston_view *wv) = 0;
};

class View : public QObject
{
public:
    explicit View(Surface *surface);
    virtual ~View();

    bool isMapped() const;
    double x() const;
    double y() const;
    QPointF pos() const;
    QRectF geometry() const;
    double alpha() const;
    void setOutput(Output *o);
    void setAlpha(double alpha);
    void setPos(double x, double y);
    void setPos(const QPointF &p) { setPos(p.x(), p.y()); }
    void setTransformParent(View *p);
    void setTransform(const Transform &tr);
    const Transform &transform() const;
    QPointF mapFromGlobal(const QPointF &p);
    QPointF mapToGlobal(const QPointF &p);

    View *mainView() const;

    void update();
    void map() { m_view->is_mapped = true; }
    void unmap();

    Output *output() const;
    Layer *layer() const { return m_layer; }
    wl_client *client() const;
    Surface *surface() const;

    static View *fromView(weston_view *v);

    View *dispatchPointerEvent(const Pointer *p, wl_fixed_t x, wl_fixed_t y);

protected:
    /**
     * Return false if the view should be transparent to pointer events,
     * true otherwise.
     */
    virtual View *pointerEnter(const Pointer *pointer) { return this; }
    virtual bool pointerLeave(const Pointer *pointer) { return true; }

private:
    explicit View(Surface *s, weston_view *view);
    static void viewDestroyed(wl_listener *listener, void *data);

    weston_view *m_view;
    ViewCreator *m_creator;
    Surface *m_surface;
    Listener *m_listener;
    Output *m_output;
    Transform *m_transform;
    struct {
        bool inside;
        View *target;
    } m_pointerState;
    Layer *m_layer;

    friend Layer;
    friend Pointer;
    friend class XWayland;
};

}

#endif
