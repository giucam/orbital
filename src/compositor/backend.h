
#ifndef ORBITAL_BACKEND_H
#define ORBITAL_BACKEND_H

#include <QObject>
#include <QHash>

class QPluginLoader;

struct weston_compositor;

namespace Orbital {

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend();

    virtual bool init(weston_compositor *c) = 0;
};

class BackendFactory
{
public:
    static void searchPlugins();
    static void cleanupPlugins();

    static Backend *createBackend(const QString &name);

private:
    QHash<QString, QPluginLoader *> m_factories;
};

}

Q_DECLARE_INTERFACE(Orbital::Backend, "Orbital.Compositor.Backend")

#endif
