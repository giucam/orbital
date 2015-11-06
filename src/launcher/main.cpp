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

class Launcher;

class Command
{
public:
    explicit Command(Launcher *l)
        : m_launcher(l)
    {
    }

    virtual void run(const QStringList &args) = 0;

    Launcher *m_launcher;
};

class KeymapCommand : public Command
{
public:
    explicit KeymapCommand(Launcher *l)
        : Command(l)
    {
    }

    void run(const QStringList &args) override;
};

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
        orbital_settings_destroy(m_settings);
        wl_registry_destroy(m_registry);
    }

    Q_INVOKABLE void create()
    {
        QQuickWindow::setDefaultAlphaBuffer(true);
        m_matcher = new MatcherModel;
        m_window = new QQuickView;
        m_window->setColor(Qt::transparent);
        m_window->setFlags(Qt::BypassWindowManagerHint);
        m_window->rootContext()->setContextProperty("availableWidth", 0.);
        m_window->rootContext()->setContextProperty("availableHeight", 0.);
        m_window->rootContext()->setContextProperty("matcherModel", m_matcher);
        m_window->setSource(QUrl("qrc:///launcher.qml"));
        connect(m_window->rootObject(), SIGNAL(selected(QString, QString)), this, SLOT(run(QString, QString)));
        m_window->show();
        wl_surface *wlSurface = static_cast<wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_window));
        m_surface = orbital_launcher_get_launcher_surface(m_launcher, wlSurface);
        orbital_launcher_surface_add_listener(m_surface, &s_launcherListener, this);

        m_matcher->setCommandPrefix(QStringLiteral(":"));
        m_matcher->addCommand(QStringLiteral("km"));
        m_commands.insert(QStringLiteral("km"), new KeymapCommand(this));
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

    orbital_settings *settings() const { return m_settings; }

private slots:
    void run(const QString &exec, const QString &fullLine)
    {
        orbital_launcher_surface_done(m_surface);
        QStringList args = fullLine.split(' ');
        if (fullLine.startsWith(':')) {
            if (args.count() < 2) {
                return;
            }

            QString command = args.at(0).mid(1);
            if (!m_commands.contains(command)) {
                return;
            }
            args.removeFirst();
            m_commands.value(command)->run(args);
        } else {
            args.removeFirst();
            if (QProcess::startDetached(exec, args)) {
                QString fullCommand = exec;
                for (const QString &arg: args) {
                    fullCommand += QString(" %1").arg(arg);
                }
                m_matcher->addInHistory(fullCommand);
            }
        }
    }

private:
    wl_display *m_display;
    wl_registry *m_registry;
    orbital_launcher *m_launcher;
    orbital_launcher_surface *m_surface;
    QQuickView *m_window;
    MatcherModel *m_matcher;
    orbital_settings *m_settings;
    QHash<QString, Command *> m_commands;

    static const wl_registry_listener s_registryListener;
    static const orbital_launcher_surface_listener s_launcherListener;
};

const wl_registry_listener Launcher::s_registryListener = {
    [](void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        Launcher *launcher = static_cast<Launcher *>(data);

#define bind(type, ver) \
        static_cast<type *>(wl_registry_bind(registry, id, &type##_interface, qMin(version, ver)));

        if (strcmp(interface, "orbital_launcher") == 0) {
            launcher->m_launcher = bind(orbital_launcher, 1u);
            QMetaObject::invokeMethod(launcher, "create");
        } else if (strcmp(interface, "orbital_settings") == 0) {
            launcher->m_settings = bind(orbital_settings, 1u);
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

    QGuiApplication app(argc, argv);
    Launcher launcher;

    return app.exec();
}

void KeymapCommand::run(const QStringList &args)
{
    if (m_launcher->settings()) {
        orbital_settings_set_keymap(m_launcher->settings(), qPrintable(args.at(0)));
    }
}

#include "main.moc"
