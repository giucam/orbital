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

#ifndef LOGINDBACKEND_H
#define LOGINDBACKEND_H

#include "loginservice.h"

class QDBusPendingCallWatcher;

class DBusInterface;

class LogindBackend : public LoginManagerBackend
{
    Q_OBJECT
public:
    ~LogindBackend();
    static LogindBackend *create();

    void poweroff() override;
    void reboot() override;
    void locked() override;
    void unlocked() override;

private slots:
    void prepareForSleep(bool v);

private:
    LogindBackend();
    void takeSleepLock();
    void getSession(QDBusPendingCallWatcher *watcher);

    DBusInterface *m_interface;
    QString m_sessionPath;
    int m_inhibitFd;
};

#endif
