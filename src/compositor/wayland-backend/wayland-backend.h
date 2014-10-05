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

#ifndef ORBITAL_WAYLAND_BACKEND_H
#define ORBITAL_WAYLAND_BACKEND_H

#include "backend.h"

namespace Orbital {

class WaylandBackend : public Backend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Orbital.Compositor.Backend" FILE "wayland-backend.json")
    Q_INTERFACES(Orbital::Backend)
public:
    WaylandBackend();

    bool init(weston_compositor *c) override;
};

}

#endif
