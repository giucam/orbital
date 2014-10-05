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
enum class PointerAxis : unsigned char;
enum class KeyboardModifiers : unsigned char;

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
    ButtonBinding(weston_compositor *c, PointerButton b, KeyboardModifiers modifiers, QObject *p = nullptr);

signals:
    void triggered(Seat *seat, uint32_t time, PointerButton button);
};

class KeyBinding : public Binding
{
    Q_OBJECT
public:
    KeyBinding(weston_compositor *c, uint32_t key, KeyboardModifiers modifiers, QObject *p = nullptr);

signals:
    void triggered(Seat *seat, uint32_t time, uint32_t key);
};

class AxisBinding : public Binding
{
    Q_OBJECT
public:
    AxisBinding(weston_compositor *c, PointerAxis axis, KeyboardModifiers modifiers, QObject *p = nullptr);

signals:
    void triggered(Seat *seat, uint32_t time, PointerAxis axis, double value);
};

}

#endif
