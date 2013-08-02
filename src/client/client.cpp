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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/input.h>

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>
#include <QDebug>
#include <QTimer>
#include <QtQml>
#include <QQuickItem>
#include <QStandardPaths>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-desktop-shell-client-protocol.h"

#include "client.h"
#include "processlauncher.h"
#include "shellitem.h"
#include "iconimageprovider.h"
#include "window.h"
#include "shellui.h"

Binding::~Binding()
{
    desktop_shell_binding_destroy(bind);
}

Client::Client()
      : QObject()
{
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
    Q_ASSERT(m_display);

    m_fd = wl_display_get_fd(m_display);
    Q_ASSERT(m_fd > -1);
    qDebug() << "Wayland display socket:" << m_fd;

    m_registry = wl_display_get_registry(m_display);
    Q_ASSERT(m_registry);

    wl_registry_add_listener(m_registry, &s_registryListener, this);


    qmlRegisterType<Binding>();
    qmlRegisterType<Window>();
}

Client::~Client()
{
    delete m_component;
    delete m_engine;
    delete m_grabWindow;
    qDeleteAll(m_bindings);
}

static const desktop_shell_binding_listener binding_listener = {
    [](void *data, desktop_shell_binding *bind) {
        Binding *b = static_cast<Binding *>(data);
        emit b->triggered();
    }
};

Binding *Client::addKeyBinding(uint32_t key, uint32_t modifiers)
{
    Binding *binding = new Binding;
    binding->bind = desktop_shell_add_key_binding(m_shell, key, modifiers);
    desktop_shell_binding_add_listener(binding->bind, &binding_listener, binding);
    m_bindings << binding;

    return binding;
}

void Client::create()
{
    m_launcher = new ProcessLauncher(this);
    m_grabWindow = new QWindow;
    m_grabWindow->setFlags(Qt::BypassWindowManagerHint);
    m_grabWindow->resize(1, 1);
    m_grabWindow->create();
    m_grabWindow->show();

    wl_surface *grabSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_grabWindow));
    desktop_shell_set_grab_surface(m_shell, grabSurface);

    QScreen *screen = QGuiApplication::screens().first();
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setAlphaBufferSize(8);
    format.setStencilBufferSize(2);

    m_engine = new QQmlEngine(this);
    m_engine->rootContext()->setContextProperty("Client", this);
    m_engine->rootContext()->setContextProperty("ProcessLauncher", m_launcher);
    m_engine->addImageProvider(QLatin1String("icon"), new IconImageProvider);

    m_ui = new ShellUI(this);
    QStringList path;
    path << QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    path << "../src/client/";
    m_ui->loadUI(m_engine, "orbital.conf", path);

    const QObjectList objects = m_ui->children();
    for (int i = 0; i < objects.size(); i++) {
        ShellItem *window = qobject_cast<ShellItem *>(objects.at(i));
        if (!window)
            continue;

        window->setFormat(format);
        window->setFlags(Qt::BypassWindowManagerHint);
        window->setScreen(screen);
        window->show();
        window->create();
        wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));

        switch (window->type()) {
            case ShellItem::Background:
                desktop_shell_set_background(m_shell, output, wlSurface);
                break;
            case ShellItem::Panel:
                desktop_shell_set_panel(m_shell, output, wlSurface);
                break;
            case ShellItem::Overlay:
                desktop_shell_add_overlay(m_shell, output, wlSurface);
                break;
            default:
                break;
        }

    }

    m_nextWindow = new Window(this);
}

void Client::createWindow()
{
    m_windows << m_nextWindow;
    connect(m_nextWindow, &Window::destroyed, this, &Client::windowRemoved);
    emit windowsChanged();

    m_nextWindow = new Window(this);
}

void Client::windowRemoved(Window *w)
{
    m_windows.removeOne(w);
    emit windowsChanged();
}

int Client::windowsCount(QQmlListProperty<Window> *prop)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_windows.count();
}

Window *Client::windowsAt(QQmlListProperty<Window> *prop, int index)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_windows.at(index);
}

QQmlListProperty<Window> Client::windows()
{
    return QQmlListProperty<Window>(this, 0, windowsCount, windowsAt);
}

void Client::requestFocus(QWindow *window)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    desktop_shell_request_focus(m_shell, wlSurface);
}

void Client::logOut()
{
    desktop_shell_quit(m_shell);
}

void Client::handleGlobal(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Q_UNUSED(version);

    Client *object = static_cast<Client *>(data);

    if (strcmp(interface, "desktop_shell") == 0) {
        // Bind interface and register listener
        object->m_shell = static_cast<desktop_shell *>(wl_registry_bind(registry, id, &desktop_shell_interface, version));
        desktop_shell_add_listener(object->m_shell, &s_shellListener, data);

        QMetaObject::invokeMethod(object, "create");
    }
}

const wl_registry_listener Client::s_registryListener = {
    Client::handleGlobal
};

void Client::configure(void *data, desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height)
{
}

void Client::handlePrepareLockSurface(void *data, desktop_shell *desktop_shell)
{
}

void Client::handleGrabCursor(void *data, desktop_shell *desktop_shell, uint32_t cursor)
{
    Client *object = static_cast<Client *>(data);
    object->m_pendingGrabCursor = cursor;
    QMetaObject::invokeMethod(object, "setGrabCursor", Qt::QueuedConnection);
}

void Client::setGrabCursor()
{
    QCursor qcursor;
    switch (m_pendingGrabCursor) {
        case DESKTOP_SHELL_CURSOR_NONE:
            break;
        case DESKTOP_SHELL_CURSOR_BUSY:
            qcursor.setShape(Qt::BusyCursor);
            break;
        case DESKTOP_SHELL_CURSOR_MOVE:
            qcursor.setShape(Qt::DragMoveCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_TOP:
            qcursor.setShape(Qt::SizeVerCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM:
            qcursor.setShape(Qt::SizeVerCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_LEFT:
            qcursor.setShape(Qt::SizeHorCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_RIGHT:
            qcursor.setShape(Qt::SizeHorCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_TOP_LEFT:
            qcursor.setShape(Qt::SizeFDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_TOP_RIGHT:
            qcursor.setShape(Qt::SizeBDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM_LEFT:
            qcursor.setShape(Qt::SizeBDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM_RIGHT:
            qcursor.setShape(Qt::SizeFDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_ARROW:
        default:
            break;
    }

    m_grabWindow->setCursor(qcursor);
}

void Client::handleWindowAdded(void *data, desktop_shell *desktop_shell, desktop_shell_window *window, const char *title, int32_t state)
{
    Client *c = static_cast<Client *>(data);
    c->m_nextWindow->init(window);
    c->m_nextWindow->setTitle(title);
    c->m_nextWindow->setState(state);

    QMetaObject::invokeMethod(c, "createWindow", Qt::QueuedConnection);
}

const desktop_shell_listener Client::s_shellListener = {
    Client::configure,
    Client::handlePrepareLockSurface,
    Client::handleGrabCursor,
    Client::handleWindowAdded
};

#include "client.moc"
