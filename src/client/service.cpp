/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include "service.h"
#include "client.h"

Q_GLOBAL_STATIC(ServiceFactory, s_factory)

Service::Service(Client *c)
       : QObject(c)
       , m_client(c)
{
}

Service *ServiceFactory::createService(const QString &name, Client *client)
{
    if (!s_factory->m_factories.contains(name)) {
        return nullptr;
    }

    Factory factory = s_factory->m_factories.value(name);
    return factory(client);
}

void ServiceFactory::registerService(const QString &name, Factory factory)
{
    s_factory->m_factories.insert(name, factory);
}
