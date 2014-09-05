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

#ifndef ORBITAL_LAYER_H
#define ORBITAL_LAYER_H

#include <QObject>

struct weston_layer;

namespace Orbital {

class View;
struct Wrapper;

class Layer : public QObject
{
    Q_OBJECT
public:
    explicit Layer(weston_layer *layer);
    explicit Layer(Layer *p = nullptr);

    void append(Layer *l);

    void addView(View *view);
    void raiseOnTop(View *view);
    void lower(View *view);

    View *topView() const;

    void setMask(int x, int y, int w, int h);

    static Layer *fromLayer(weston_layer *layer);

private:
    Wrapper *m_layer;

};

}

#endif

