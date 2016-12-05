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
#include <windowed-output-api.h>

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

    if (weston_compositor_load_backend(c, WESTON_BACKEND_WAYLAND, &config.base) != 0) {
        return false;
    }

    const struct weston_windowed_output_api *api = weston_windowed_output_get_api(c);
    if (!api) {
        qWarning("Cannot use weston_windowed_output_api.");
        return false;
    }

    m_pendingListener.setNotify([api](Listener *, void *data) {
        auto output = static_cast<weston_output *>(data);

        weston_output_set_scale(output, 1);
        weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);
        api->output_set_size(output, 800, 400);
        weston_output_enable(output);
    });
    m_pendingListener.connect(&c->output_pending_signal);

    api->output_create(c, "WL1");
    return true;
}

}
