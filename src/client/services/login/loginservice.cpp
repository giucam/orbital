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

#include <QTimer>

#include "loginservice.h"
#include "client.h"
#include "clibackend.h"
#ifdef USE_LOGIND
#include "logindbackend.h"
#endif

LoginService::LoginService()
            : Service()
            , m_backend(nullptr)
{
}

LoginService::~LoginService()
{
    delete m_backend;
}

void LoginService::init()
{
#ifdef USE_LOGIND
    m_backend = LogindBackend::create();
#endif
    if (!m_backend) {
        m_backend = CliBackend::create();
    }
}

void LoginService::logOut()
{
    client()->quit();
}

void LoginService::poweroff()
{
    client()->quit();
    if (m_backend) {
        m_backend->poweroff();
    }
}

void LoginService::reboot()
{
    client()->quit();
    if (m_backend) {
        m_backend->reboot();
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
