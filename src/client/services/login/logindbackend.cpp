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

#include <QDBusInterface>

#include "logindbackend.h"

LogindBackend::LogindBackend()
{

}

LogindBackend::~LogindBackend()
{
    delete m_interface;
}

LogindBackend *LogindBackend::create()
{
    LogindBackend *logind = new LogindBackend();
    if (!logind) {
        return nullptr;
    }

    logind->m_interface = new QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1",
                                             "org.freedesktop.login1.Manager", QDBusConnection::systemBus());
    if (!logind->m_interface || !logind->m_interface->isValid()) {
        delete logind;
        return nullptr;
    }
    return logind;
}

void LogindBackend::poweroff()
{
    m_interface->call("PowerOff", true);
}

void LogindBackend::reboot()
{
    m_interface->call("Reboot", true);
}
