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
#include <QProcess>

#include "loginservice.h"
#include "client.h"

LoginService::LoginService()
            : Service()
            , m_interface(nullptr)
{
}

LoginService::~LoginService()
{
    delete m_interface;
}

void LoginService::init()
{
    m_interface = new QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1",
                                     "org.freedesktop.login1.Manager", QDBusConnection::systemBus());
    if (!m_interface || !m_interface->isValid()) {
        m_interface = nullptr;
    }
}

void LoginService::logOut()
{
    client()->quit();
}

void LoginService::poweroff()
{
    client()->quit();
    if (m_interface) {
        m_interface->call("PowerOff", true);
    } else {
        // try the dumb fallback
        QProcess::execute("shutdown -h now");
    }
}

void LoginService::reboot()
{
    client()->quit();
    if (m_interface) {
        m_interface->call("Reboot", true);
    } else {
        // try the dumb fallback
        QProcess::execute("shutdown -r now");
    }
}

void LoginService::requestLogOut()
{
    m_request = &LoginService::logOut;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit logOutRequested();
}

void LoginService::requestPoweroff()
{
    m_request = &LoginService::poweroff;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit poweroffRequested();
}

void LoginService::requestReboot()
{
    m_request = &LoginService::reboot;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit rebootRequested();
}

void LoginService::requestHandled()
{
    m_requestHandled = true;
}

void LoginService::doRequest()
{
    if (!m_requestHandled) {
        (this->*m_request)();
    }
}
