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


ButtonBinding::ButtonBinding(weston_compositor *c, PointerButton b, QObject *p)
             : Binding(p)
{
    auto handler = [](weston_seat *s, uint32_t time, uint32_t button, void *data) {
        Seat *seat = Seat::fromSeat(s);
        emit static_cast<ButtonBinding *>(data)->triggered(seat, time, rawToPointerButton(button));
    };
    m_binding = weston_compositor_add_button_binding(c, pointerButtonToRaw(b), (weston_keyboard_modifier)0, handler, this);
}

}
