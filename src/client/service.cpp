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

#include <QDebug>
#include <QPluginLoader>
#include <QJsonObject>
#include <QDir>
#include <QObject>

#include "service.h"
#include "client.h"

Q_GLOBAL_STATIC(ServiceFactory, s_factory)

Service::Service()
       : QObject()
       , m_client(nullptr)
{
}


void ServiceFactory::searchPlugins()
{
    QDir pluginsDir(QLatin1String(LIBRARIES_PATH) + "/services");
    for (const QString &fileName: pluginsDir.entryList(QDir::Files)) {
        QPluginLoader *pluginLoader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        QJsonObject metaData = pluginLoader->metaData();
        if (metaData.value("IID").toString() == QLatin1String("Orbital.Service")) {
            QString name = metaData.value("className").toString();
            s_factory->m_factories.insert(name, pluginLoader);
        } else {
            delete pluginLoader;
        }
    }
}

void ServiceFactory::cleanupPlugins()
{
    for (QPluginLoader *p: s_factory->m_factories) {
        delete p;
    }
}

Service *ServiceFactory::createService(const QString &name, Client *client)
{
    if (!s_factory->m_factories.contains(name)) {
        qWarning() << "Cannot find the plugin" << name;
        return nullptr;
    }

    QPluginLoader *loader = s_factory->m_factories.value(name);
    QObject *obj = loader->instance();
    if (!obj) {
        qWarning() << "Could not load the plugin" << name;
        qWarning() << loader->errorString();
        return nullptr;
    }

    Service *s = qobject_cast<Service *>(obj);
    if (!s) {
        qWarning() << "The plugin" << name << "is not a Service subclass!";
        return nullptr;
    }

    s->m_client = client;
    s->init();

    return s;
}
