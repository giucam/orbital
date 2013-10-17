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
#include <QQmlListProperty>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-desktop-shell-client-protocol.h"

#include "client.h"
#include "iconimageprovider.h"
#include "window.h"
#include "shellui.h"
#include "element.h"
#include "grab.h"
#include "workspace.h"
#include "utils.h"
#include "uiscreen.h"
#include "service.h"
#include "style.h"

Client *Client::s_client = nullptr;

class ClientPrivate {
    Q_DECLARE_PUBLIC(Client)
public:
    ClientPrivate(Client *c) : locale(QLocale::system()), q_ptr(c) {}

    QQmlListProperty<Window> windows();
    QQmlListProperty<Workspace> workspaces();
    QQmlListProperty<ElementInfo> elementsInfo();
    QQmlListProperty<StyleInfo> stylesInfo();

    static int windowsCount(QQmlListProperty<Window> *prop);
    static Window *windowsAt(QQmlListProperty<Window> *prop, int index);
    static int workspacesCount(QQmlListProperty<Workspace> *prop);
    static Workspace *workspacesAt(QQmlListProperty<Workspace> *prop, int index);

    QLocale locale;
    Client *q_ptr;
};

Binding::~Binding()
{
    desktop_shell_binding_destroy(bind);
}

Client::Client()
      : QObject()
      , d_ptr(new ClientPrivate(this))
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
    qmlRegisterType<Grab>();
    qmlRegisterType<Style>("Orbital", 1, 0, "Style");
    qmlRegisterUncreatableType<Window>("Orbital", 1, 0, "Window", "Cannot create Window");
    qmlRegisterUncreatableType<Workspace>("Orbital", 1, 0, "Workspace", "Cannot create Workspace");
    qmlRegisterUncreatableType<ElementInfo>("Orbital", 1, 0, "ElementInfo", "ElementInfo is not creatable");
    qmlRegisterUncreatableType<StyleInfo>("Orbital", 1, 0, "StyleInfo", "StyleInfo is not creatable");

#define REGISTER_QMLFILE(type) qmlRegisterType(QUrl::fromLocalFile(QString(":/qml/") + type + ".qml"), "Orbital", 1, 0, type)
    REGISTER_QMLFILE("Icon");
    REGISTER_QMLFILE("ElementConfiguration");
    REGISTER_QMLFILE("ElementsChooser");
    REGISTER_QMLFILE("Spacer");
    REGISTER_QMLFILE("Element");
    REGISTER_QMLFILE("Button");
    REGISTER_QMLFILE("Rotator");

    QTranslator *tr = new QTranslator;
    if (tr->load(d_ptr->locale, "", "", DATA_PATH "/translations", ".qm")) {
        QCoreApplication::installTranslator(tr);
    } else {
        delete tr;
    }

    QCoreApplication::setApplicationName("orbital");
    QQuickWindow::setDefaultAlphaBuffer(true);

    Element::loadElementsList();
    Style::loadStylesList();
    ServiceFactory::searchPlugins();
}

Client::~Client()
{
    delete m_grabWindow;
    delete m_ui;
    qDeleteAll(m_workspaces);
    qDeleteAll(m_services);

    Element::cleanupElementsList();
    Style::cleanupStylesList();
    ServiceFactory::cleanupPlugins();
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

    return binding;
}

void Client::create()
{
    m_grabWindow = new QWindow;
    m_grabWindow->setFlags(Qt::BypassWindowManagerHint);
    m_grabWindow->resize(1, 1);
    m_grabWindow->create();
    m_grabWindow->show();

    wl_surface *grabSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_grabWindow));
    desktop_shell_set_grab_surface(m_shell, grabSurface);

    QQmlEngine *engine = new QQmlEngine(this);
    engine->rootContext()->setContextProperty("Client", this);
    engine->addImageProvider(QLatin1String("icon"), new IconImageProvider);

    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString configFile = path + "/orbital.conf";
    m_ui = new ShellUI(this, engine, configFile);

    for (int i = 0; i < QGuiApplication::screens().size(); ++i) {
        QScreen *screen = QGuiApplication::screens().at(i);

        m_ui->loadScreen(i, screen);
        qDebug() << "Elements for screen" << i << "loaded after" << m_elapsedTimer.elapsed() << "ms";
    }

    // wait until all the objects have finished what they're doing before sending the ready event
    while (QCoreApplication::hasPendingEvents()) {
        QCoreApplication::processEvents();
    }
    ready();
}

void Client::setBackground(QQuickWindow *window, QScreen *screen)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    desktop_shell_set_background(m_shell, output, wlSurface);
}

void Client::setPanel(QQuickWindow *window, QScreen *screen, int location)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    desktop_shell_set_panel(m_shell, output, wlSurface, location);
}

void Client::addOverlay(QQuickWindow *window, QScreen *screen)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    desktop_shell_add_overlay(m_shell, output, wlSurface);
}

void Client::setInputRegion(QQuickWindow *w, const QRectF &r)
{
    wl_compositor *compositor = static_cast<wl_compositor *>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("compositor"));
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", w));

    wl_region *region = wl_compositor_create_region(compositor);
    wl_region_add(region, r.x(), r.y(), r.width(), r.height());
    wl_surface_set_input_region(wlSurface, region);
    wl_region_destroy(region);
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

int ClientPrivate::windowsCount(QQmlListProperty<Window> *prop)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_windows.count();
}

Window *ClientPrivate::windowsAt(QQmlListProperty<Window> *prop, int index)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_windows.at(index);
}

QQmlListProperty<Window> ClientPrivate::windows()
{
    return QQmlListProperty<Window>(q_ptr, 0, windowsCount, windowsAt);
}

int ClientPrivate::workspacesCount(QQmlListProperty<Workspace> *prop)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_workspaces.count();
}

Workspace *ClientPrivate::workspacesAt(QQmlListProperty<Workspace> *prop, int index)
{
    Client *c = static_cast<Client *>(prop->object);
    return c->m_workspaces.at(index);
}

QQmlListProperty<Workspace> ClientPrivate::workspaces()
{
    return QQmlListProperty<Workspace>(q_ptr, 0, workspacesCount, workspacesAt);
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

QQmlListProperty<ElementInfo> ClientPrivate::elementsInfo()
{
    return QQmlListProperty<ElementInfo>(q_ptr, 0, elementsInfoCount, elementsInfoAt);
}

static int stylesInfoCount(QQmlListProperty<StyleInfo> *prop)
{
    return Style::stylesInfo().count();
}

static StyleInfo *stylesInfoAt(QQmlListProperty<StyleInfo> *prop, int index)
{
    const QString &name = Style::stylesInfo().keys().at(index);
    return Style::stylesInfo().value(name);
}

QQmlListProperty<StyleInfo> ClientPrivate::stylesInfo()
{
    return QQmlListProperty<StyleInfo>(q_ptr, 0, stylesInfoCount, stylesInfoAt);
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

QLocale Client::locale()
{
    return s_client->d_ptr->locale;
}

void Client::quit()
{
    desktop_shell_quit(m_shell);
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

desktop_shell_surface *Client::setPopup(QWindow *w)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", w));
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", w->screen()));
    return desktop_shell_set_popup(m_shell, output, wlSurface, w->x(), w->y());
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
    wrapInterface(&Client::handleGlobal),
    [](void *, wl_registry *registry, uint32_t id) {}
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

QQuickWindow *Client::window(Element *elm)
{
    if (elm->type() == ElementInfo::Type::Item)
        return nullptr;

    for (QQuickWindow *w: m_uiWindows) {
        if (w->property("element").value<Element *>() == elm) {
            return w;
        }
    }

    QQuickWindow *window = new QQuickWindow();
    elm->setParentItem(window->contentItem());
    window->setProperty("element", QVariant::fromValue(elm));
    m_uiWindows << window;

    return window;
}

QQuickWindow *Client::createUiWindow()
{
    QQuickWindow *window = new QQuickWindow();

    window->setColor(Qt::transparent);
    window->create();

    return window;
}

#include "moc_client.cpp"
