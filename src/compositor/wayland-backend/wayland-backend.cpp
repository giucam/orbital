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

#include <compositor-wayland.h>

#include "wayland-backend.h"

namespace Orbital {

WaylandBackend::WaylandBackend()
{

}

bool WaylandBackend::init(weston_compositor *c)
{
    weston_wayland_backend_config config;
    config.base.struct_version = WESTON_WAYLAND_BACKEND_CONFIG_VERSION;
    config.base.struct_size = sizeof(config);
    config.use_pixman = false;
    config.sprawl = false;
    config.display_name = nullptr;
    config.fullscreen = false;
    config.cursor_theme = nullptr;
    config.cursor_size = 32;

    weston_wayland_backend_output_config outputs[] = {
        {   800, 400, "WL1", 0, 1 },
    };
    config.num_outputs = sizeof(outputs) / sizeof(weston_wayland_backend_output_config);
    config.outputs = outputs;

    int ret = weston_compositor_load_backend(c, WESTON_BACKEND_WAYLAND, &config.base);
    return ret == 0;
}

}
