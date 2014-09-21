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

#ifndef ORBITAL_DESKTOP_SHELL_WINDOW_H
#define ORBITAL_DESKTOP_SHELL_WINDOW_H

#include <wayland-server.h>

#include "interface.h"

namespace Orbital {

class ShellSurface;
class DesktopShell;
class Seat;

class DesktopShellWindow : public Interface
{
    Q_OBJECT
public:
    DesktopShellWindow(DesktopShell *ds);
    ~DesktopShellWindow();

    void recreate();

protected:
    virtual void added() override;

private:
    ShellSurface *shsurf();
    void create();
    void surfaceTypeChanged();
    void activated(Seat *seat);
    void deactivated(Seat *seat);
    void minimized();
    void restored();
    void mapped();
    void destroy();
    void sendState();
    void sendTitle();
    void setState(wl_client *client, wl_resource *resource, int32_t state);
    void close(wl_client *client, wl_resource *resource);

    DesktopShell *m_desktopShell;
    wl_resource *m_resource;
    int32_t m_state;
    bool m_sendState;
};

}

#endif
