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

#ifndef ORBITAL_WLSHELL_H
#define ORBITAL_WLSHELL_H

#include "../interface.h"

struct wl_resource;

namespace Orbital {

class Shell;
class Compositor;
class ShellSurface;

class WlShell : public Interface, public Global
{
    Q_OBJECT
public:
    explicit WlShell(Shell *shell, Compositor *c);

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    ShellSurface *getShellSurface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surface_resource);
//     void pointerFocus(ShellSeat *seat, weston_pointer *pointer);
//     void surfaceResponsiveness(WlShellSurface *shsurf);

//     static void sendConfigure(weston_surface *surface, int32_t width, int32_t height);

    Shell *m_shell;

    static const struct wl_shell_interface shell_implementation;
    static const struct weston_shell_client shell_client;
};

}

#endif