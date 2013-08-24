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

#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QHash>

class Client;
class QPluginLoader;

class Service : public QObject
{
    Q_OBJECT
public:
    Service();
    virtual void init() {};

protected:
    Client *client() const { return m_client; }

private:
    Client *m_client;

    friend class ServiceFactory;
};

Q_DECLARE_INTERFACE(Service, "Orbital.Service")

class ServiceFactory
{
public:
    static void searchPlugins();
    static void cleanupPlugins();

    static Service *createService(const QString &name, Client *client);

private:
    QHash<QString, QPluginLoader *> m_factories;
};

#endif
