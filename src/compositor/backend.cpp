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

#include <QPluginLoader>
#include <QDir>
#include <QDebug>
#include <QJsonArray>

#include "backend.h"

namespace Orbital {

Q_GLOBAL_STATIC(BackendFactory, s_factory)

Backend::Backend()
{
}


void BackendFactory::searchPlugins()
{
    QDir pluginsDir(QStringLiteral(LIBRARIES_PATH "/compositor/backends"));
    foreach (const QString &fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader *pluginLoader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        QJsonObject metaData = pluginLoader->metaData();
        if (metaData.value(QStringLiteral("IID")).toString() == QStringLiteral("Orbital.Compositor.Backend")) {
            const QStringList keys = metaData.value(QStringLiteral("MetaData")).toObject().toVariantMap().value(QStringLiteral("Keys")).toStringList();
            for (QString key: keys) {
                s_factory->m_factories[key.toStdString()] = pluginLoader;
            }
        } else {
            delete pluginLoader;
        }
    }
}

void BackendFactory::cleanupPlugins()
{
    std::for_each(s_factory->m_factories.begin(), s_factory->m_factories.end(),
                  [](const std::pair<std::string, QPluginLoader *> &pair) {
                      delete pair.second;
                  });
}

Backend *BackendFactory::createBackend(StringView name)
{
    auto it = s_factory->m_factories.find(name.toStdString());
    if (it == std::end(s_factory->m_factories)) {
        qWarning() << "Cannot find the plugin" << name;
        return nullptr;
    }

    QPluginLoader *loader = it->second;
    QObject *obj = loader->instance();
    if (!obj) {
        qWarning() << "Could not load the plugin" << name;
        qWarning() << loader->errorString();
        return nullptr;
    }

    Backend *b = qobject_cast<Backend *>(obj);
    if (!b) {
        qWarning() << "The plugin" << name << "is not a Backend subclass!";
        return nullptr;
    }

    return b;
}

}
