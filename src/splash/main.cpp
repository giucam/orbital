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

#include <stdlib.h>

#include <QGuiApplication>
#include <QList>
#include <QQuickView>
#include <QScreen>
#include <QDebug>
#include <QTranslator>
#include <qpa/qplatformnativeinterface.h>

#include "wayland-desktop-shell-client-protocol.h"

class Splash : public QObject
{
    Q_OBJECT
public:
    Splash()
        : QObject()
        , m_splash(nullptr)
    {
        QTranslator *tr = new QTranslator;
        if (tr->load(QLocale::system(), "", "", DATA_PATH "/translations", ".qm")) {
            QCoreApplication::installTranslator(tr);
        } else {
            delete tr;
        }

        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
        m_registry = wl_display_get_registry(m_display);
        wl_registry_add_listener(m_registry, &s_registryListener, this);

        QQuickWindow::setDefaultAlphaBuffer(true);
        for (QScreen *screen: QGuiApplication::screens()) {
            QQuickView *w = new QQuickView(QUrl("qrc:///splash.qml"));
            w->setColor(Qt::transparent);
            w->setResizeMode(QQuickView::SizeRootObjectToView);
            w->setScreen(screen);
            w->resize(screen->size());
            w->setFlags(Qt::BypassWindowManagerHint);
            w->show();
            m_windows << w;
        }

        wl_display_roundtrip(m_display);
        if (!m_splash) {
            exit(1);
        }
    }
    ~Splash()
    {
        desktop_shell_splash_destroy(m_splash);
        wl_registry_destroy(m_registry);
        wl_display_disconnect(m_display);
    }

    Q_INVOKABLE void create()
    {
        desktop_shell_splash_add_listener(m_splash, &s_splashListener, this);
        for (int i = 0; i < QGuiApplication::screens().size(); ++i) {
            QScreen *screen = QGuiApplication::screens().at(i);
            wl_surface *wlSurface = static_cast<wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_windows.at(i)));
            wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));
            desktop_shell_splash_set_surface(m_splash, output, wlSurface);
        }
    }

private:
    wl_display *m_display;
    wl_registry *m_registry;
    desktop_shell_splash *m_splash;
    QList<QQuickView *> m_windows;

    static const wl_registry_listener s_registryListener;
    static const desktop_shell_splash_listener s_splashListener;
};

const wl_registry_listener Splash::s_registryListener = {
    [](void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        Splash *splash = static_cast<Splash *>(data);

        if (strcmp(interface, "desktop_shell_splash") == 0) {
            splash->m_splash = static_cast<desktop_shell_splash *>(wl_registry_bind(registry, id, &desktop_shell_splash_interface, version));
            QMetaObject::invokeMethod(splash, "create");
        }
    },
    [](void *, wl_registry *registry, uint32_t id) {}
};

const desktop_shell_splash_listener Splash::s_splashListener = {
    [](void *data, desktop_shell_splash *splash) {
        QCoreApplication::quit();
    }
};

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    setenv("QT_MESSAGE_PATTERN", "[orbital-splash %{type}] %{message}", 0);

    QGuiApplication app(argc, argv);
    Splash splash;

    return app.exec();
}

#include "main.moc"
