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

#ifndef ORBITAL_WLSHELLSURFACE_H
#define ORBITAL_WLSHELLSURFACE_H

#include "interface.h"

struct wl_resource;

namespace Orbital {

class WlShell;
class ShellSurface;

class WlShellSurface : public Interface
{
    Q_OBJECT
public:
    WlShellSurface(WlShell *shell, ShellSurface *shsurf, wl_client *client, uint32_t id);

private:
    void resourceDestroyed();
    ShellSurface *shellSurface() const;
    void popupDone();

    void pong(uint32_t serial);
    void move(wl_resource *seat_resource, uint32_t serial);
    void resize(wl_resource *seat_resource, uint32_t serial, uint32_t edges);
    void setToplevel();
    void setTransient(wl_resource *parent_resource, int x, int y, uint32_t flags);
    void setFullscreen(uint32_t method, uint32_t framerate, wl_resource *output_resource);
    void setPopup(wl_resource *seat_resource, uint32_t serial, wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags);
    void setMaximized(wl_resource *output_resource);
    void setTitle(const char *title);
    void setClass(const char *className);

    WlShell *m_wlShell;
    wl_resource *m_resource;
};

}

#endif
