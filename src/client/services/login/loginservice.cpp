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

#include <QDBusInterface>

#include "loginservice.h"
#include "client.h"

LoginService::LoginService()
            : Service()
{
}

LoginService::~LoginService()
{

}

void LoginService::init()
{
    m_interface = new QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1",
                                     "org.freedesktop.login1.Manager", QDBusConnection::systemBus());
}

void LoginService::logOut()
{
    client()->quit();
}

void LoginService::poweroff()
{
    client()->quit();
    m_interface->call("PowerOff", true);
}

void LoginService::reboot()
{
    client()->quit();
    m_interface->call("Reboot", true);
}
