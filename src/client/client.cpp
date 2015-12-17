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

#include <linux/input.h>
#include <sys/socket.h>
#include <unistd.h>

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
#include <QProcess>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-desktop-shell-client-protocol.h"
#include "wayland-settings-client-protocol.h"
#include "wayland-clipboard-client-protocol.h"

#include "client.h"
#include "iconimageprovider.h"
#include "window.h"
#include "shellui.h"
#include "element.h"
#include "grab.h"
#include "workspace.h"
#include "utils.h"
#include "uiscreen.h"
#include "style.h"
#include "notification.h"
#include "compositorsettings.h"
#include "activeregion.h"
#include "clipboard.h"

Client *Client::s_client = nullptr;

const QEvent::Type PingEventType = QEvent::Type(QEvent::User + 1);

class PingEvent : public QEvent
{
public:
    PingEvent(uint32_t s) : QEvent(PingEventType), serial(s) {}
    uint32_t serial;
};

class ClientPrivate {
    Q_DECLARE_PUBLIC(Client)
public:
    ClientPrivate(Client *c) : locale(QLocale::system()), q_ptr(c), locked(false) {}

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
    bool locked;
};

Binding::~Binding()
{
    desktop_shell_binding_destroy(bind);
}

Client::Client()
      : QObject()
      , m_notifications(nullptr)
      , m_settings(nullptr)
      , m_ui(nullptr)
      , d_ptr(new ClientPrivate(this))
{
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

    qmlRegisterType<Grab>();
    qmlRegisterType<Style>("Orbital", 1, 0, "Style");
    qmlRegisterType<NotificationWindow>("Orbital", 1, 0, "NotificationWindow");
    qmlRegisterType<ActiveRegion>("Orbital", 1, 0, "ActiveRegion");
    qmlRegisterUncreatableType<Window>("Orbital", 1, 0, "Window", QStringLiteral("Cannot create Window"));
    qmlRegisterUncreatableType<Workspace>("Orbital", 1, 0, "Workspace", QStringLiteral("Cannot create Workspace"));
    qmlRegisterUncreatableType<ElementInfo>("Orbital", 1, 0, "ElementInfo", QStringLiteral("ElementInfo is not creatable"));
    qmlRegisterUncreatableType<StyleInfo>("Orbital", 1, 0, "StyleInfo", QStringLiteral("StyleInfo is not creatable"));
    qmlRegisterUncreatableType<UiScreen>("Orbital", 1, 0, "UiScreen", QStringLiteral("UiScreen is not creatable"));
    qmlRegisterUncreatableType<Clipboard>("Orbital", 1, 0, "Clipboard", QStringLiteral("Clipboard is only available via attached properties"));

    qRegisterMetaType<QScreen *>();

#define REGISTER_QMLFILE(type) qmlRegisterType(QUrl::fromLocalFile(QStringLiteral(":/qml/" type ".qml")), "Orbital", 1, 0, type)
    REGISTER_QMLFILE("Icon");
    REGISTER_QMLFILE("ElementConfiguration");
    REGISTER_QMLFILE("ElementsChooser");
    REGISTER_QMLFILE("Spacer");
    REGISTER_QMLFILE("Element");
    REGISTER_QMLFILE("Button");
    REGISTER_QMLFILE("Rotator");
    REGISTER_QMLFILE("PopupElement");

    QTranslator *tr = new QTranslator;
    if (tr->load(d_ptr->locale, QString(), QString(), QStringLiteral(DATA_PATH "/translations"), QStringLiteral(".qm"))) {
        QCoreApplication::installTranslator(tr);
    } else {
        delete tr;
    }

    QQuickWindow::setDefaultAlphaBuffer(true);

    Element::loadElementsList();
    Style::loadStylesList();

    m_engine = new QQmlEngine(this);
    m_engine->rootContext()->setContextProperty(QStringLiteral("Client"), this);
    m_engine->addImageProvider(QStringLiteral("icon"), new IconImageProvider);
    m_engine->addImportPath(QStringLiteral(LIBRARIES_PATH "/qml"));

    // TODO: find a way to un-hardcode this
    QQmlComponent *c = new QQmlComponent(m_engine, QUrl(QStringLiteral("qrc:/qml/Notifications.qml")), this);
    if (!c->isReady()) {
        qDebug() << c->errorString();
    } else {
        c->create();
    }
    // -- till here
}

Client::~Client()
{
    delete m_grabWindow;
    delete m_ui;
    delete m_settings;
    qDeleteAll(m_workspaces);

    Element::cleanupElementsList();
    Style::cleanupStylesList();
}

enum BindingMods {
    BindingModsNone = 0,
    BindingModsCtrl = (1 << 0),
    BindingModsAlt = (1 << 1),
    BindingModsSuper = (1 << 2),
    BindingModsShift = (1 << 3),
};

Binding *Client::addKeyBinding(uint32_t key, Qt::KeyboardModifiers modifiers, const std::function<void (wl_seat *)> &cb)
{
    int mods = BindingModsNone;
    if (modifiers & Qt::ShiftModifier)
        mods |= BindingModsShift;
    if (modifiers & Qt::ControlModifier)
        mods |= BindingModsCtrl;
    if (modifiers & Qt::AltModifier)
        mods |= BindingModsAlt;
    if (modifiers & Qt::MetaModifier)
        mods |= BindingModsSuper;

    static const desktop_shell_binding_listener binding_listener = {
        [](void *data, desktop_shell_binding *bind, wl_seat *seat) {
            Binding *b = static_cast<Binding *>(data);
            b->callback(seat);
        }
    };

    Binding *binding = new Binding;
    binding->bind = desktop_shell_add_key_binding(m_shell, key, mods);
    binding->callback = cb;
    desktop_shell_binding_add_listener(binding->bind, &binding_listener, binding);

    return binding;
}

void Client::create()
{
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &Client::screenAdded);
    foreach(QScreen *s, qGuiApp->screens()) {
        screenAdded(s);
    }

    m_grabWindow = new QWindow;
    m_grabWindow->setFlags(Qt::BypassWindowManagerHint);
    m_grabWindow->resize(1, 1);
    m_grabWindow->create();
    m_grabWindow->show();
    connect(m_grabWindow, &QWindow::screenChanged, [this](QScreen *s) { setGrabSurface(); });
    setGrabSurface();
}

void Client::loadOutput(QScreen *s, const QString &name, uint32_t serial)
{
    m_elapsedTimer.start();
    if (!m_ui) {
        QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QString configFile = path + "/orbital/orbital.conf";
        m_ui = new ShellUI(this, m_settings, m_engine, configFile);
    }
    UiScreen *screen = m_ui->loadScreen(s, name);
    qDebug() << "Elements for screen" << name << "loaded after" << m_elapsedTimer.elapsed() << "ms";

    connect(screen, &UiScreen::loaded, [this, serial]() { sendOutputLoaded(serial); });
}

void Client::sendOutputLoaded(uint32_t serial)
{
    desktop_shell_output_loaded(m_shell, serial);
}

void Client::setBackground(QQuickWindow *window, QScreen *screen)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    desktop_shell_set_background(m_shell, output, wlSurface);
}

desktop_shell_panel *Client::setPanel(QQuickWindow *window, QScreen *screen, int location)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window));
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));
    if (!m_uiWindows.contains(window)) {
        addUiWindow(window);
    }

    return desktop_shell_set_panel(m_shell, output, wlSurface, location);
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

void Client::setLockScreen(QQuickWindow *window, QScreen *screen)
{
    wl_surface *surface = nativeSurface(window);
    wl_output *output = nativeOutput(screen);
    desktop_shell_set_lock_surface(m_shell, surface, output);
}

void Client::windowDestroyed(Window *w)
{
    m_windows.removeOne(w);
    emit windowsChanged();
    emit windowRemoved(w);
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
    QString name = Element::elementsInfo().keys().at(index);
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
    QString name = Style::stylesInfo().keys().at(index);
    return Style::stylesInfo().value(name);
}

QQmlListProperty<StyleInfo> ClientPrivate::stylesInfo()
{
    return QQmlListProperty<StyleInfo>(q_ptr, 0, stylesInfoCount, stylesInfoAt);
}

QLocale Client::locale()
{
    return s_client->d_ptr->locale;
}

void Client::quit()
{
    desktop_shell_quit(m_shell);
}

void Client::lockSession()
{
    desktop_shell_lock(m_shell);
}

void Client::unlockSession()
{
    desktop_shell_unlock(m_shell);
    d_ptr->locked = false;
    emit unlocked();
}

bool Client::isSessionLocked() const
{
    return d_ptr->locked;
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
        desktop_shell_workspace *workspace = desktop_shell_add_workspace(m_shell);

        Workspace *ws = new Workspace(workspace);
        m_workspaces << ws;
        emit workspacesChanged();
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

void Client::selectWorkspace(UiScreen *screen, Workspace *ws)
{
    desktop_shell_select_workspace(m_shell, nativeOutput(screen->screen()), ws->m_workspace);
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

desktop_shell_surface *Client::setPopup(QWindow *w, QWindow *parent)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", w));
    wl_surface *parentSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", parent));
    return desktop_shell_set_popup(m_shell, parentSurface, wlSurface, w->x(), w->y());
}

notification_surface *Client::pushNotification(QWindow *w, bool inactive)
{
    if (m_notifications) {
        return notifications_manager_push_notification(m_notifications, nativeSurface(w), (int)inactive);
    }
    return nullptr;
}

active_region *Client::createActiveRegion(QQuickWindow *w, const QRect &region)
{
    wl_surface *wlSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", w));
    return desktop_shell_create_active_region(m_shell, wlSurface, region.x(), region.y(), region.width(), region.height());
}

wl_subsurface *Client::getSubsurface(QQuickWindow *window, QQuickWindow *parent)
{
    return wl_subcompositor_get_subsurface(m_subcompositor, nativeSurface(window), nativeSurface(parent));
}

void Client::handleGlobal(wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Q_UNUSED(version);

    if (strcmp(interface, "desktop_shell") == 0) {
        // Bind interface and register listener
        m_shell = static_cast<desktop_shell *>(wl_registry_bind(registry, id, &desktop_shell_interface, version));
        desktop_shell_add_listener(m_shell, &s_shellListener, this);
    } else if (strcmp(interface, "nuclear_settings") == 0) {
        m_settings = new CompositorSettings(static_cast<nuclear_settings *>(wl_registry_bind(registry, id, &nuclear_settings_interface, version)));
        m_settings->moveToThread(QCoreApplication::instance()->thread());
    } else if (strcmp(interface, "notifications_manager") == 0) {
        m_notifications = static_cast<notifications_manager *>(wl_registry_bind(registry, id, &notifications_manager_interface, 1));
    } else if (strcmp(interface, "wl_subcompositor") == 0) {
        m_subcompositor = static_cast<wl_subcompositor *>(wl_registry_bind(registry, id, &wl_subcompositor_interface, 1));
    } else if (strcmp(interface, "orbital_clipboard_manager") == 0) {
        wl_registry_bind(registry, id, &orbital_clipboard_manager_interface, 1);
    }
}

const wl_registry_listener Client::s_registryListener = {
    wrapInterface(&Client::handleGlobal),
    [](void *, wl_registry *registry, uint32_t id) {}
};

void Client::handlePing(desktop_shell *shell, uint32_t serial)
{
    // catch the case where the event thread spins freely but the gui thread is stuck
    qApp->postEvent(this, new PingEvent(serial));
}

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
        case DESKTOP_SHELL_CURSOR_KILL:
            qcursor.setShape(Qt::ForbiddenCursor);
            break;
        case DESKTOP_SHELL_CURSOR_ARROW:
        default:
            break;
    }

    m_grabWindow->setCursor(qcursor);
}

void Client::handleWindowAdded(desktop_shell *desktop_shell, desktop_shell_window *window, uint32_t pid)
{
    Window *w = new Window(window, pid);

    m_windows << w;
    w->moveToThread(QCoreApplication::instance()->thread());

    connect(w, &Window::destroyed, this, &Client::windowDestroyed);

    emit windowsChanged();
    emit windowAdded(w);
}

void Client::handleWorkspaceAdded(desktop_shell *desktop_shell, desktop_shell_workspace *workspace)
{
    Workspace *ws = new Workspace(workspace);

    m_workspaces << ws;
    ws->moveToThread(QCoreApplication::instance()->thread());
    emit workspacesChanged();
}

void Client::handleDesktopRect(desktop_shell *desktop_shell, wl_output *output, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (!m_ui) {
        return;
    }

    UiScreen *screen = m_ui->findScreen(output);
    if (screen) {
        screen->setAvailableRect(QRect(x, y, width, height));
    } else {
        qWarning() << "Client::handleDesktopRect: Could not find a UiScreen for a wl_output!";
    }
}

void Client::handleLocked(desktop_shell *desktop_shell)
{
    d_ptr->locked = true;
    emit locked();
}

void Client::handleCompositorAction(desktop_shell *, orbital_compositor_action *act, const char *name)
{
    addAction(QByteArray("Compositor.") + name, [act, this](wl_seat *seat) { orbital_compositor_action_run(act, seat); wl_display_flush(m_display); });
}

const desktop_shell_listener Client::s_shellListener = {
    wrapInterface(&Client::handlePing),
    wrapInterface(&Client::handleLoad),
    wrapInterface(&Client::handleConfigure),
    wrapInterface(&Client::handlePrepareLockSurface),
    wrapInterface(&Client::handleGrabCursor),
    wrapInterface(&Client::handleWindowAdded),
    wrapInterface(&Client::handleWorkspaceAdded),
    wrapInterface(&Client::handleDesktopRect),
    wrapInterface(&Client::handleLocked),
    wrapInterface(&Client::handleCompositorAction),
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

    foreach (QQuickWindow *w, m_uiWindows) {
        if (w->property("element").value<Element *>() == elm) {
            return w;
        }
    }

    QQuickWindow *window = new QQuickWindow();
    elm->setParentItem(window->contentItem());
    window->setProperty("element", QVariant::fromValue(elm));
    addUiWindow(window);

    return window;
}

void Client::addUiWindow(QQuickWindow *window)
{
    m_uiWindows << window;
    connect(window, &QObject::destroyed, [this](QObject *o) { m_uiWindows.removeOne(static_cast<QQuickWindow *>(o)); });
}

QQuickWindow *Client::createUiWindow()
{
    QQuickWindow *window = new QQuickWindow();

    window->setColor(Qt::transparent);
    window->create();

    return window;
}

void Client::setGrabSurface()
{
    wl_surface *s = nativeSurface(m_grabWindow);
    if (s) {
        desktop_shell_set_grab_surface(m_shell, s);
    }
}

void Client::screenAdded(QScreen *s)
{
    class Feedback
    {
    public:
        Feedback(Client *c, QScreen *s, desktop_shell_output_feedback *f)
            : client(c)
            , screen(s)
            , feedback(f)
        {
            static const desktop_shell_output_feedback_listener listener = {
                wrapInterface(&Feedback::load)
            };
            desktop_shell_output_feedback_add_listener(feedback, &listener, this);
        }

        void load(desktop_shell_output_feedback *, const char *name, uint32_t serial)
        {
            QMetaObject::invokeMethod(client, "loadOutput", Q_ARG(QScreen *, screen), Q_ARG(QString, QString(name)), Q_ARG(uint32_t, serial));

            desktop_shell_output_feedback_destroy(feedback);
            delete this;
        }

        Client *client;
        QScreen *screen;
        desktop_shell_output_feedback *feedback;
    };

    new Feedback(this, s, desktop_shell_output_bound(m_shell, nativeOutput(s)));
}

bool Client::event(QEvent *e)
{
    switch (e->type()) {
        case PingEventType: {
            PingEvent *pe = static_cast<PingEvent *>(e);
            desktop_shell_pong(m_shell, pe->serial);
            wl_display_flush(m_display);
        } return true;
        default:
            break;
    };
    return QObject::event(e);
}

void Client::addAction(const QByteArray &name, const std::function<void (wl_seat *)> &action)
{
    if (m_actions.contains(name)) {
        qWarning("Action '%s' already exists.", name.constData());
        return;
    }

    m_actions.insert(name, action);
}

std::function<void (wl_seat *)> *Client::action(const QByteArray &name)
{
    if (!m_actions.contains(name)) {
        qWarning("Action '%s' not found. Available actions are:", name.constData());
        qWarning() << m_actions.keys();
        return nullptr;
    }

    return &m_actions[name];
}

wl_output *Client::nativeOutput(QScreen *screen)
{
    return static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));
}

wl_surface *nativeSurface(QWindow *w)
{
    return static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", w));
}

#include "moc_client.cpp"
