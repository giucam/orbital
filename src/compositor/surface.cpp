/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <weston/compositor.h>

#include "surface.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    Surface *surface;
};

Surface::Surface(weston_surface *surface, QObject *p)
       : Object(p)
       , m_surface(surface)
       , m_role(nullptr)
       , m_configureHandler(nullptr)
       , m_listener(new Listener)
       , m_activable(true)
{
    if (surface->configure) {
        qFatal("Error: trying to create a Surface for an already taken weston_surface.");
    }

    m_listener->listener.notify = [](wl_listener *listener, void *data)
    {
        Surface *surface = reinterpret_cast<Listener *>(listener)->surface;
        surface->m_surface = nullptr;
        delete surface;
    };
    m_listener->surface = this;
    wl_signal_add(&surface->destroy_signal, &m_listener->listener);

    surface->configure_private = this;
    surface->configure = configure;
}

Surface::~Surface()
{
    wl_list_remove(&m_listener->listener.link);
    if (m_surface) {
        weston_surface_destroy(m_surface);
    }
}

wl_client *Surface::client() const
{
    return m_surface->resource ? wl_resource_get_client(m_surface->resource) : nullptr;
}

weston_surface *Surface::surface() const
{
    return m_surface;
}

bool Surface::isMapped() const
{
    return weston_surface_is_mapped(m_surface);
}

void Surface::repaint()
{
    weston_surface_schedule_repaint(m_surface);
}

void Surface::damage()
{
    weston_surface_damage(m_surface);
}

void Surface::setRole(Role *role, const ConfigureHandler &handler)
{
    if (m_role && m_role != role) {
        qWarning("The surface has a different role already.");
        return;
    }

    m_role = role;
    m_configureHandler = handler;
}

Surface::Role *Surface::role() const
{
    return m_role;
}

Surface::ConfigureHandler Surface::configureHandler() const
{
    return m_configureHandler;
}

void Surface::setActivable(bool activable)
{
    m_activable = activable;
}

void Surface::ref()
{
    m_surface->ref_count++;
}

void Surface::deref()
{
    weston_surface_destroy(m_surface);
}

Surface *Surface::activate(Seat *seat)
{
    return this;
}

Surface *Surface::fromSurface(weston_surface *surf)
{
    if (surf->configure == configure) {
        return static_cast<Surface *>(surf->configure_private);
    } else if (!surf->configure && !surf->configure_private) {
        return new Surface(surf);
    }

    return nullptr;
}

Surface *Surface::fromResource(wl_resource *res)
{
    weston_surface *surf = static_cast<weston_surface *>(wl_resource_get_user_data(res));
    return fromSurface(surf);
}

void Surface::configure(weston_surface *s, int32_t x, int32_t y)
{
    Surface *surf = static_cast<Surface *>(s->configure_private);
    if (surf->m_role && surf->m_configureHandler) {
        surf->m_configureHandler(x, y);
    }
}

}
