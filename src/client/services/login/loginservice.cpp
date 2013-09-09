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

    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &LoginService::decreaseTimeout);
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

void LoginService::requestLogOut()
{
    startRequest(&LoginService::logOut, "logout");
}

void LoginService::requestPoweroff()
{
    startRequest(&LoginService::poweroff, "poweroff");
}

void LoginService::requestReboot()
{
    startRequest(&LoginService::reboot, "reboot");
}

void LoginService::abortRequest()
{
    m_timer.stop();
}

void LoginService::startRequest(void (LoginService::*request)(), const QString &op)
{
    m_request = request;
    m_timeout = 3;

    emit timeoutStarted(op);
    emit timeoutChanged();

    m_timer.start();
}

void LoginService::decreaseTimeout()
{
    --m_timeout;
    if (m_timeout < 1) {
        (this->*m_request)();
    } else {
        emit timeoutChanged();
    }
}
