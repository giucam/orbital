/*
 * Copyright 2016 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_WDESKTOP_H
#define ORBITAL_WDESKTOP_H

#include <libweston-desktop.h>

#include "../interface.h"

namespace Orbital {

class Shell;
class Pointer;

class WDesktop : public Interface
{
public:
    explicit WDesktop(Shell *shell, Compositor *c);
    ~WDesktop();

private:
    void pingTimeout(weston_desktop_client *client);
    void pong(weston_desktop_client *client);
    void surfaceAdded(weston_desktop_surface *surface);
    void surfaceRemoved(weston_desktop_surface *surface);
    void committed(weston_desktop_surface *surface, int32_t sx, int32_t sy);
    void showWindowMenu(weston_desktop_surface *surface, weston_seat *seat, int32_t x, int32_t y);
    void setParent(weston_desktop_surface *surface, weston_desktop_surface *parent);
    void move(weston_desktop_surface *surface, weston_seat *seat, uint32_t serial);
    void resize(weston_desktop_surface *surface, weston_seat *seat, uint32_t serial, weston_desktop_surface_edge edges);
    void fullscreenRequested(weston_desktop_surface *surface, bool fullscreen, weston_output *output);
    void maximizedRequested(weston_desktop_surface *surface, bool maximized);
    void minimizedRequested(weston_desktop_surface *surface);

    void pointerFocus(Pointer *pointer);

    Compositor *m_compositor;
    weston_desktop *m_wdesktop;
    Shell *m_shell;
};

}

#endif
