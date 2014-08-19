
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
    QDir pluginsDir(QLatin1String(LIBRARIES_PATH) + "/compositor/backends");
    for (const QString &fileName: pluginsDir.entryList(QDir::Files)) {
        QPluginLoader *pluginLoader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        QJsonObject metaData = pluginLoader->metaData();
        if (metaData.value("IID").toString() == QLatin1String("Orbital.Compositor.Backend")) {
            QStringList keys = metaData.value("MetaData").toObject().toVariantMap().value("Keys").toStringList();
            for (QString key: keys) {
                s_factory->m_factories.insert(key, pluginLoader);
            }
        } else {
            delete pluginLoader;
        }
    }
}

void BackendFactory::cleanupPlugins()
{
    for (QPluginLoader *p: s_factory->m_factories) {
        delete p;
    }
}

Backend *BackendFactory::createBackend(const QString &name)
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

    Backend *b = qobject_cast<Backend *>(obj);
    if (!b) {
        qWarning() << "The plugin" << name << "is not a Backend subclass!";
        return nullptr;
    }

    return b;
}

}
