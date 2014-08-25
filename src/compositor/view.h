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
struct weston_view;

namespace Orbital
{

class Output;
class Layer;
class WorkspaceView;
class Pointer;
struct Listener;

class View
{
public:
    explicit View(weston_view *view);
    virtual ~View();

    bool isMapped() const;
    double x() const;
    double y() const;
    QRectF geometry() const;
    void setOutput(Output *o);
    void setPos(double x, double y);
    void setPos(const QPointF &p) { setPos(p.x(), p.y()); }
    void setTransformParent(View *p);
    QPointF mapFromGlobal(const QPointF &p);

    void update();

    Output *output() const;
    wl_client *client() const;
    weston_surface *surface() const;

    static View *fromView(weston_view *v);

private:
    weston_view *m_view;
    Listener *m_listener;
    Output *m_output;

    friend Layer;
    friend Pointer;
};

}

#endif
