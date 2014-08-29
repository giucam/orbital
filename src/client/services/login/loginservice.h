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

#ifndef LOGINSERVICE_H
#define LOGINSERVICE_H

#include "service.h"

class LoginService : public Service
{
    Q_OBJECT
    Q_INTERFACES(Service)
    Q_PLUGIN_METADATA(IID "Orbital.Service")

public:
    class Backend
    {
    public:
        virtual ~Backend() {}

        virtual void poweroff() = 0;
        virtual void reboot() = 0;
    };

    LoginService();
    ~LoginService();

    void init();

public slots:
    void abort();
    void logOut();
    void poweroff();
    void reboot();
    void requestLogOut();
    void requestPoweroff();
    void requestReboot();
    void requestHandled();

signals:
    void logOutRequested();
    void poweroffRequested();
    void rebootRequested();
    void aborted();

private slots:
    void doRequest();

private:
    Backend *m_backend;
    void (LoginService::*m_request)();
    bool m_requestHandled;
};

#endif
