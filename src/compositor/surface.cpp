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

#include <compositor.h>

#include "surface.h"
#include "view.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    Surface *surface;
};

void Surface::destroy(wl_listener *listener, void *data)
{
    Surface *surface = reinterpret_cast<Listener *>(listener)->surface;
    surface->m_surface = nullptr;
    delete surface;
}

Surface::Surface(weston_surface *surface, QObject *p)
       : Object(p)
       , m_surface(surface)
       , m_roleHandler(nullptr)
       , m_listener(new Listener)
       , m_activable(true)
       , m_workspaceMask(-1)
{
    m_listener->listener.notify = destroy;
    m_listener->surface = this;
    wl_signal_add(&surface->destroy_signal, &m_listener->listener);
}

Surface::~Surface()
{
    emit unmapped();

    qDeleteAll(m_views);
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

bool Surface::setRole(const char *roleName, wl_resource *errorResource, uint32_t errorCode)
{
    if (weston_surface_set_role(m_surface, roleName, errorResource, errorCode) == 0) {
        m_surface->configure_private = this;
        m_surface->configure = configure;
        return true;
    }
    return false;
}

void Surface::setRole(const char *roleName)
{
    m_surface->role_name = roleName;
    m_surface->configure_private = this;
    m_surface->configure = configure;
}

void Surface::setRoleHandler(RoleHandler *handler)
{
    if (m_roleHandler) {
        m_roleHandler->surface = nullptr;
    }
    m_roleHandler = handler;
    if (handler) {
        handler->surface = this;
    }
}

const char *Surface::role() const
{
    return m_surface->role_name;
}

Surface::RoleHandler *Surface::roleHandler() const
{
    return m_roleHandler;
}

void Surface::setWorkspaceMask(int mask)
{
    m_workspaceMask = mask;
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

Surface *Surface::activate()
{
    return this;
}

void Surface::move(Seat *seat)
{
    if (m_surface->configure == configure && m_roleHandler) {
        m_roleHandler->move(seat);
    }
}

Surface *Surface::fromSurface(weston_surface *surf)
{
    if (surf->configure == configure) {
        return static_cast<Surface *>(surf->configure_private);
    } else if (wl_listener *listener = wl_signal_get(&surf->destroy_signal, destroy)) {
        return reinterpret_cast<Listener *>(listener)->surface;
    }

    return new Surface(surf);
}

Surface *Surface::fromResource(wl_resource *res)
{
    weston_surface *surf = static_cast<weston_surface *>(wl_resource_get_user_data(res));
    return fromSurface(surf);
}

void Surface::configure(weston_surface *s, int32_t x, int32_t y)
{
    Surface *surf = static_cast<Surface *>(s->configure_private);
    if (surf->m_roleHandler) {
        surf->m_roleHandler->configure(x, y);
    }
}

Surface::RoleHandler::~RoleHandler()
{
    if (surface) {
        surface->m_roleHandler = nullptr;
    }
}

}
