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
#include <QQuickWindow>
#include <QDBusInterface>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-desktop-shell-client-protocol.h"

#include "client.h"
#include "processlauncher.h"
#include "iconimageprovider.h"
#include "window.h"
#include "shellui.h"
#include "element.h"
#include "grab.h"
#include "workspace.h"
#include "utils.h"
#include "uiscreen.h"
#include "service.h"
#include "volumecontrol.h"

Client *Client::s_client = nullptr;

Binding::~Binding()
{
    desktop_shell_binding_destroy(bind);
}

Client::Client()
      : QObject()
{
    m_elapsedTimer.start();
    s_client = this;

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
    qmlRegisterType<Service>();
    qmlRegisterUncreatableType<Window>("Orbital", 1, 0, "Window", "Cannot create Window");
    qmlRegisterUncreatableType<Workspace>("Orbital", 1, 0, "Workspace", "Cannot create Workspace");
    qmlRegisterUncreatableType<ElementInfo>("Orbital", 1, 0, "ElementInfo", "ElementInfo is not creatable");

#define REGISTER_QMLFILE(type) qmlRegisterType(QUrl::fromLocalFile(QString(":/qml/") + type + ".qml"), "Orbital", 1, 0, type)
    REGISTER_QMLFILE("Button");
    REGISTER_QMLFILE("ElementConfiguration");
    REGISTER_QMLFILE("ElementsChooser");
    REGISTER_QMLFILE("Spacer");

    m_loginServiceInterface = new QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1",
                                                 "org.freedesktop.login1.Manager", QDBusConnection::systemBus());

    QCoreApplication::setApplicationName("orbital");
    QQuickWindow::setDefaultAlphaBuffer(true);

    Element::loadElementsList();
}

Client::~Client()
{
    delete m_grabWindow;
    qDeleteAll(m_bindings);
    qDeleteAll(m_workspaces);
    qDeleteAll(m_services);

    Element::cleanupElementsList();
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

    QQmlEngine *engine = new QQmlEngine(this);
    engine->rootContext()->setContextProperty("Client", this);
    engine->rootContext()->setContextProperty("ProcessLauncher", m_launcher);
    engine->addImageProvider(QLatin1String("icon"), new IconImageProvider);

    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString configFile = path + "/orbital.conf";
    m_ui = new ShellUI(this, engine, configFile);

    for (int i = 0; i < QGuiApplication::screens().size(); ++i) {
        QScreen *screen = QGuiApplication::screens().at(i);
        wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

        UiScreen *sc = m_ui->loadScreen(i);
        qDebug() << "Elements for screen" << i << "loaded after" << m_elapsedTimer.elapsed() << "ms";

        const QObjectList objects = sc->children();
        for (int i = 0; i < objects.size(); i++) {
            Element *elm = qobject_cast<Element *>(objects.at(i));
            if (!elm)
                continue;

            if (elm->type() == ElementInfo::Type::Item)
                continue;


            QQuickWindow *window = new QQuickWindow();
            elm->setParentItem(window->contentItem());

            window->setWidth(elm->width());
            window->setHeight(elm->height());
            window->setColor(Qt::transparent);
            window->setFlags(Qt::BypassWindowManagerHint);
            window->setScreen(screen);
            window->show();
            window->create();
            m_uiWindows << window;
            wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));

            switch (elm->type()) {
                case ElementInfo::Type::Background:
                    desktop_shell_set_background(m_shell, output, wlSurface);
                    break;
                case ElementInfo::Type::Panel:
                    desktop_shell_set_panel(m_shell, output, wlSurface);
                    break;
                case ElementInfo::Type::Overlay:
                    desktop_shell_add_overlay(m_shell, output, wlSurface);
                    break;
                default:
                    break;
            }

        }
    }

    // wait until all the objects have finished what they're doing before sending the ready event
    while (QCoreApplication::hasPendingEvents()) {
        QCoreApplication::processEvents();
    }
    ready();
}

void Client::ready()
{
    desktop_shell_desktop_ready(m_shell);
    qDebug() << "Orbital-client startup time:" << m_elapsedTimer.elapsed() << "ms";
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

int Client::workspacesCount(QQmlListProperty<Workspace> *prop)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_workspaces.count();
}

Workspace *Client::workspacesAt(QQmlListProperty<Workspace> *prop, int index)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_workspaces.at(index);
}

QQmlListProperty<Workspace> Client::workspaces()
{
    return QQmlListProperty<Workspace>(this, 0, workspacesCount, workspacesAt);
}

static int elementsInfoCount(QQmlListProperty<ElementInfo> *prop)
{
    return Element::elementsInfo().count();
}

static ElementInfo *elementsInfoAt(QQmlListProperty<ElementInfo> *prop, int index)
{
    const QString &name = Element::elementsInfo().keys().at(index);
    return Element::elementsInfo().value(name);
}

QQmlListProperty<ElementInfo> Client::elementsInfo()
{
    return QQmlListProperty<ElementInfo>(this, 0, elementsInfoCount, elementsInfoAt);
}

void Client::requestFocus(QWindow *window)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    desktop_shell_request_focus(m_shell, wlSurface);
}

Service *Client::service(const QString &name)
{
    Service *s = m_services.value(name);
    if (s) {
        return s;
    }

    s = ServiceFactory::createService(name, this);
    m_services.insert(name, s);
    return s;
}

void Client::logOut()
{
    desktop_shell_quit(m_shell);
}

void Client::poweroff()
{
    logOut();
    m_loginServiceInterface->call("PowerOff", true);
}

void Client::reboot()
{
    logOut();
    m_loginServiceInterface->call("Reboot", true);
}

void Client::minimizeWindows()
{
    desktop_shell_minimize_windows(m_shell);
}

void Client::restoreWindows()
{
    desktop_shell_restore_windows(m_shell);
}

void Client::addWorkspace(int n)
{
    if (m_workspaces.size() <= n) {
        desktop_shell_add_workspace(m_shell);
    }
}

void Client::removeWorkspace(int n)
{
    Workspace *ws = m_workspaces.takeAt(n);
    if (ws) {
        emit workspacesChanged();
        delete ws;
    }
}

void Client::selectWorkspace(Workspace *ws)
{
    desktop_shell_select_workspace(m_shell, ws->m_workspace);
}

QQuickWindow *Client::findWindow(wl_surface *surface) const
{
    for (QQuickWindow *w: m_uiWindows) {
        wl_surface *surf = static_cast<wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", w));
        if (surf == surface) {
            return w;
        }
    }

    return nullptr;
}

void Client::handleGlobal(wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Q_UNUSED(version);

    if (strcmp(interface, "desktop_shell") == 0) {
        // Bind interface and register listener
        m_shell = static_cast<desktop_shell *>(wl_registry_bind(registry, id, &desktop_shell_interface, version));
        desktop_shell_add_listener(m_shell, &s_shellListener, this);
    }
}

const wl_registry_listener Client::s_registryListener = {
    wrapInterface(&Client::handleGlobal)
};

void Client::handleLoad(desktop_shell *shell)
{
    QMetaObject::invokeMethod(this, "create");
}

void Client::handleConfigure(desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height)
{
}

void Client::handlePrepareLockSurface(desktop_shell *desktop_shell)
{
}

void Client::handleGrabCursor(desktop_shell *desktop_shell, uint32_t cursor)
{
    m_pendingGrabCursor = cursor;
    QMetaObject::invokeMethod(this, "setGrabCursor", Qt::QueuedConnection);
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

void Client::handleWindowAdded(desktop_shell *desktop_shell, desktop_shell_window *window, const char *title, int32_t state)
{
    Window *w = new Window();
    w->init(window, state);
    w->setTitle(title);

    m_windows << w;
    w->moveToThread(QCoreApplication::instance()->thread());

    connect(w, &Window::destroyed, this, &Client::windowRemoved);

    emit windowsChanged();
}

void Client::handleWorkspaceAdded(desktop_shell *desktop_shell, desktop_shell_workspace *workspace, int active)
{
    Workspace *ws = new Workspace(workspace);
    ws->m_active = active;

    m_workspaces << ws;
    ws->moveToThread(QCoreApplication::instance()->thread());
    emit workspacesChanged();
}

const desktop_shell_listener Client::s_shellListener = {
    wrapInterface(&Client::handleLoad),
    wrapInterface(&Client::handleConfigure),
    wrapInterface(&Client::handlePrepareLockSurface),
    wrapInterface(&Client::handleGrabCursor),
    wrapInterface(&Client::handleWindowAdded),
    wrapInterface(&Client::handleWorkspaceAdded)
};

Grab *Client::createGrab()
{
    desktop_shell_grab *grab = desktop_shell_start_grab(s_client->m_shell);
    return new Grab(grab);
}

QQuickWindow *Client::createUiWindow()
{
    QQuickWindow *window = new QQuickWindow();

    window->setColor(Qt::transparent);
    window->create();

    return window;
}

#include "client.moc"
