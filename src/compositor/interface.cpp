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

#include <wayland-server.h>

#include "interface.h"
#include "compositor.h"

namespace Orbital {

Object::Object(QObject *p)
      : QObject(p)
{
}

Object::~Object()
{
    qDeleteAll(m_ifaces);
}

void Object::addInterface(Interface *iface)
{
    m_ifaces.push_back(iface);
    iface->m_obj = this;
    iface->added();
}



Interface::Interface(QObject *p)
         : QObject(p)
         , m_obj(nullptr)
{
}


Global::Global(Compositor *c, const wl_interface *i, uint32_t v)
{
    m_global = wl_global_create(c->m_display, i, v, this,
                                [](wl_client *client, void *data, uint32_t version, uint32_t id) {
                                    emit static_cast<Global *>(data)->bind(client, version, id);
                                });
}

Global::~Global()
{
    wl_global_destroy(m_global);
}

}
