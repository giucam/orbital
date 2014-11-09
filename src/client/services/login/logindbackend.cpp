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

#include <unistd.h>

#include <QDebug>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include "logindbackend.h"

const static QString s_login1Service = QStringLiteral("org.freedesktop.login1");
const static QString s_login1Path = QStringLiteral("/org/freedesktop/login1");
const static QString s_login1ManagerInterface = QStringLiteral("org.freedesktop.login1.Manager");
const static QString s_login1SessionInterface = QStringLiteral("org.freedesktop.login1.Session");

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

    logind->m_interface = new QDBusInterface(s_login1Service, s_login1Path,
                                             s_login1ManagerInterface, QDBusConnection::systemBus());
    if (!logind->m_interface || !logind->m_interface->isValid()) {
        delete logind;
        return nullptr;
    }

    logind->m_interface->connection().connect(s_login1Service, s_login1Path, s_login1ManagerInterface,
                                              QStringLiteral("PrepareForSleep"), logind, SIGNAL(requestLock()));

    QDBusPendingCall call = logind->m_interface->asyncCall("GetSessionByPID", (quint32)getpid());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    logind->connect(watcher, &QDBusPendingCallWatcher::finished, logind, &LogindBackend::getSession);
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

void LogindBackend::getSession(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    watcher->deleteLater();
    if (!reply.isValid()) {
        return;
    }
    m_sessionPath = reply.value().path();
    qDebug() << "Session" << m_sessionPath;

    m_interface->connection().connect(s_login1Service, m_sessionPath, s_login1SessionInterface,
                                      QStringLiteral("Lock"), this, SIGNAL(requestLock()));
    m_interface->connection().connect(s_login1Service, m_sessionPath, s_login1SessionInterface,
                                      QStringLiteral("Unlock"), this, SIGNAL(requestUnlock()));
}
