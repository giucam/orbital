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

#ifndef ORBITAL_BINDING_H
#define ORBITAL_BINDING_H

#include <QObject>

#include <wayland-server.h>

struct weston_binding;
struct weston_compositor;

namespace Orbital {

class Seat;
enum class PointerButton : unsigned char;

class Binding : public QObject
{
    Q_OBJECT
public:
    explicit Binding(QObject *p = nullptr);
    virtual ~Binding();

protected:
    weston_binding *m_binding;
};

class ButtonBinding : public Binding
{
    Q_OBJECT
public:
    ButtonBinding(weston_compositor *c, PointerButton b, QObject *p = nullptr);

public:

signals:
    void triggered(Seat *seat, uint32_t time, PointerButton button);
};

}

#endif
