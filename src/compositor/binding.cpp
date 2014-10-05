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

#include <weston/compositor.h>

#include "binding.h"
#include "seat.h"
#include "global.h"

namespace Orbital {

Binding::Binding(QObject *p)
       : QObject(p)
       , m_binding(nullptr)
{
}

Binding::~Binding()
{
    weston_binding_destroy(m_binding);
}


ButtonBinding::ButtonBinding(weston_compositor *c, PointerButton b, KeyboardModifiers modifiers, QObject *p)
             : Binding(p)
{
    auto handler = [](weston_seat *s, uint32_t time, uint32_t button, void *data) {
        Seat *seat = Seat::fromSeat(s);
        emit static_cast<ButtonBinding *>(data)->triggered(seat, time, rawToPointerButton(button));
    };
    m_binding = weston_compositor_add_button_binding(c, pointerButtonToRaw(b), (weston_keyboard_modifier)modifiers, handler, this);
}



KeyBinding::KeyBinding(weston_compositor *c, uint32_t key, KeyboardModifiers modifiers, QObject *p)
          : Binding(p)
{
    auto handler = [](weston_seat *s, uint32_t time, uint32_t key, void *data) {
        Seat *seat = Seat::fromSeat(s);
        emit static_cast<KeyBinding *>(data)->triggered(seat, time, key);
    };
    m_binding = weston_compositor_add_key_binding(c, key, (weston_keyboard_modifier)modifiers, handler, this);
}



AxisBinding::AxisBinding(weston_compositor *c, PointerAxis axis, KeyboardModifiers modifiers, QObject *p)
           : Binding(p)
{
    auto handler = [](weston_seat *s, uint32_t time, uint32_t axis, wl_fixed_t value, void *data) {
        Seat *seat = Seat::fromSeat(s);
        emit static_cast<AxisBinding *>(data)->triggered(seat, time, (PointerAxis)axis, wl_fixed_to_double(value));
    };
    m_binding = weston_compositor_add_axis_binding(c, (uint32_t)axis, (weston_keyboard_modifier)modifiers, handler, this);
}

}
