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

#ifndef LOGINSERVICE_H
#define LOGINSERVICE_H

#include <QTimer>

#include "service.h"

class QDBusInterface;

class LoginService : public Service
{
    Q_OBJECT
    Q_INTERFACES(Service)
    Q_PLUGIN_METADATA(IID "Orbital.Service")

    Q_PROPERTY(int timeout READ timeout NOTIFY timeoutChanged)
public:
    LoginService();
    ~LoginService();

    void init();

    int timeout() const { return m_timeout; }

public slots:
    void logOut();
    void poweroff();
    void reboot();
    void requestLogOut();
    void requestPoweroff();
    void requestReboot();
    void abortRequest();

signals:
    void timeoutStarted(const QString &operation);
    void timeoutChanged();

private:
    void startRequest(void (LoginService::*request)(), const QString &op);
    void decreaseTimeout();

    QDBusInterface *m_interface;
    int m_timeout;
    void (LoginService::*m_request)();
    QTimer m_timer;
};

#endif
