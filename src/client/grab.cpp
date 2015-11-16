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
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>

#include "wayland-desktop-shell-client-protocol.h"

#include "grab.h"
#include "utils.h"
#include "client.h"

Grab::Grab(desktop_shell_grab *g)
    : QObject()
    , m_grab(g)
{
    desktop_shell_grab_add_listener(g, &s_desktop_shell_grab_listener, this);
}

Grab::~Grab()
{
    desktop_shell_grab_destroy(m_grab);
}

void Grab::end()
{
    desktop_shell_grab_end(m_grab);
}

void Grab::handleEnded(desktop_shell_grab *grab)
{
    emit ended();
}

void Grab::handleFocus(desktop_shell_grab *grab, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    emit focus(Client::client()->findWindow(surface), wl_fixed_to_int(x), wl_fixed_to_int(y));
}

void Grab::handleMotion(desktop_shell_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    emit motion(time, wl_fixed_to_int(x), wl_fixed_to_int(y));
}

void Grab::handleButton(desktop_shell_grab *grab, uint32_t time , uint32_t btn, uint32_t state)
{
    emit button(time, btn, state);
}

const struct desktop_shell_grab_listener Grab::s_desktop_shell_grab_listener = {
    wrapInterface(&Grab::handleEnded),
    wrapInterface(&Grab::handleFocus),
    wrapInterface(&Grab::handleMotion),
    wrapInterface(&Grab::handleButton)
};
