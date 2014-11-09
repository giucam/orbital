/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <stdlib.h>

#include <QGuiApplication>
#include <QList>
#include <QQuickView>
#include <QScreen>
#include <QDebug>
#include <QQmlContext>
#include <QProcess>
#include <QQuickItem>
#include <qpa/qplatformnativeinterface.h>

#include "matchermodel.h"
#include "wayland-desktop-shell-client-protocol.h"

static const QEvent::Type ConfigureEventType = (QEvent::Type)QEvent::registerEventType();

class ConfigureEvent : public QEvent
{
public:
    ConfigureEvent(int w, int h)
        : QEvent(ConfigureEventType)
        , width(w)
        , height(h)
        { }

    int width;
    int height;
};


class Launcher : public QObject
{
    Q_OBJECT
public:
    Launcher()
        : QObject()
        , m_launcher(nullptr)
    {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
        m_registry = wl_display_get_registry(m_display);
        wl_registry_add_listener(m_registry, &s_registryListener, this);

        wl_callback *callback = wl_display_sync(m_display);
        static const wl_callback_listener callbackListener = {
            [](void *data, wl_callback *c, uint32_t) {
                Launcher *launcher = reinterpret_cast<Launcher *>(data);
                if (!launcher->m_launcher) {
                    qFatal("The compositor doesn't advertize the orbital_launcher global.");
                }
                wl_callback_destroy(c);
            }
        };
        wl_callback_add_listener(callback, &callbackListener, this);
    }
    ~Launcher()
    {
        orbital_launcher_destroy(m_launcher);
        wl_registry_destroy(m_registry);
    }

    Q_INVOKABLE void create()
    {
        QQuickWindow::setDefaultAlphaBuffer(true);
        m_window = new QQuickView;
        m_window->setColor(Qt::transparent);
        m_window->setFlags(Qt::BypassWindowManagerHint);
        m_window->rootContext()->setContextProperty("availableWidth", 0.);
        m_window->rootContext()->setContextProperty("availableHeight", 0.);
        m_window->rootContext()->setContextProperty("matcherModel", new MatcherModel);
        m_window->setSource(QUrl("qrc:///launcher.qml"));
        connect(m_window->rootObject(), SIGNAL(selected(QString)), this, SLOT(run(QString)));
        m_window->show();
        wl_surface *wlSurface = static_cast<wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_window));
        m_surface = orbital_launcher_get_launcher_surface(m_launcher, wlSurface);
        orbital_launcher_surface_add_listener(m_surface, &s_launcherListener, this);
    }

    bool event(QEvent *e) override
    {
        if (e->type() == ConfigureEventType) {
            ConfigureEvent *ce = static_cast<ConfigureEvent *>(e);
            m_window->rootContext()->setContextProperty("availableWidth", ce->width);
            m_window->rootContext()->setContextProperty("availableHeight", ce->height);
            QMetaObject::invokeMethod(m_window->rootObject(), "reset");
            m_window->update();
            return true;
        }
        return QObject::event(e);
    }

private slots:
    void run(const QString &exec)
    {
        QProcess::startDetached(exec);
        orbital_launcher_surface_done(m_surface);
    }

private:
    wl_display *m_display;
    wl_registry *m_registry;
    orbital_launcher *m_launcher;
    orbital_launcher_surface *m_surface;
    QQuickView *m_window;

    static const wl_registry_listener s_registryListener;
    static const orbital_launcher_surface_listener s_launcherListener;
};

const wl_registry_listener Launcher::s_registryListener = {
    [](void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        Launcher *launcher = static_cast<Launcher *>(data);

        if (strcmp(interface, "orbital_launcher") == 0) {
            launcher->m_launcher = static_cast<orbital_launcher *>(wl_registry_bind(registry, id, &orbital_launcher_interface, 1));
            QMetaObject::invokeMethod(launcher, "create");
        }
    },
    [](void *, wl_registry *registry, uint32_t id) {}
};

const orbital_launcher_surface_listener Launcher::s_launcherListener = {
    [](void *data, orbital_launcher_surface *l, int width, int height)
    {
        Launcher *launcher = static_cast<Launcher *>(data);
        qApp->postEvent(launcher, new ConfigureEvent(width, height));
    }
};

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    setenv("QT_MESSAGE_PATTERN", "[orbital-launcher %{type}] %{message}", 0);

    QGuiApplication app(argc, argv);
    Launcher launcher;

    return app.exec();
}

#include "main.moc"
