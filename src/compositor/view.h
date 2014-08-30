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

#include <QRectF>

struct wl_client;
struct wl_listener;
struct weston_view;

namespace Orbital
{

class Output;
class Layer;
class WorkspaceView;
class Pointer;
class Transform;
struct Listener;

class View
{
public:
    explicit View(weston_view *view);
    virtual ~View();

    bool isMapped() const;
    double x() const;
    double y() const;
    QPointF pos() const;
    QRectF geometry() const;
    void setOutput(Output *o);
    void setAlpha(double alpha);
    void setPos(double x, double y);
    void setPos(const QPointF &p) { setPos(p.x(), p.y()); }
    void setTransformParent(View *p);
    void setTransform(const Transform &tr);
    QPointF mapFromGlobal(const QPointF &p);

    void update();
    void unmap();

    Output *output() const;
    Layer *layer() const;
    wl_client *client() const;
    weston_surface *surface() const;

    static View *fromView(weston_view *v);

private:
    static void viewDestroyed(wl_listener *listener, void *data);

    weston_view *m_view;
    Listener *m_listener;
    Output *m_output;
    Transform *m_transform;

    friend Layer;
    friend Pointer;
    friend class XWayland;
};

}

#endif
