/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>

#include "wayland-desktop-shell-client-protocol.h"

#include "grab.h"

Grab::Grab(desktop_shell_grab *g)
    : QObject()
    , m_grab(g)
{
    desktop_shell_grab_add_listener(g, &s_desktop_shell_grab_listener, this);
}

void Grab::end()
{
    desktop_shell_grab_end(m_grab);
}

void Grab::desktop_shell_grab_focus(void *data, desktop_shell_grab *grab, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    emit static_cast<Grab *>(data)->focus(surface, wl_fixed_to_int(x), wl_fixed_to_int(y));
}

void Grab::desktop_shell_grab_motion(void *data, desktop_shell_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    emit static_cast<Grab *>(data)->motion(time, wl_fixed_to_int(x), wl_fixed_to_int(y));
}

void Grab::desktop_shell_grab_button(void *data, desktop_shell_grab *grab, uint32_t time , uint32_t button, uint32_t state)
{
    emit static_cast<Grab *>(data)->button(time, button, state);
}

const struct desktop_shell_grab_listener Grab::s_desktop_shell_grab_listener = {
    desktop_shell_grab_focus,
    desktop_shell_grab_motion,
    desktop_shell_grab_button
};
