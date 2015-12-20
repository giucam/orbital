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

#include <weston-1/compositor-wayland.h>

#include "wayland-backend.h"

namespace Orbital {

WaylandBackend::WaylandBackend()
{

}

bool WaylandBackend::init(weston_compositor *c)
{
    const char *display_name = NULL;
    int use_pixman = 0;
    int sprawl = 0;

    wayland_backend *b = wayland_backend_create(c, use_pixman, display_name, NULL, 32, sprawl);
    if (!b)
        return false;

    if (!sprawl) {
        wayland_output *output = wayland_output_create(b, 0, 0, 800, 400, "WL1", 0, 0, 1);
        wayland_output_set_windowed(output);
    }

    return true;
}

}
