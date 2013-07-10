
#include <linux/input.h>

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>
#include <QDebug>
#include <QTimer>
#include <QtQml>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-desktop-shell-client-protocol.h"

#include "client.h"
#include "processlauncher.h"
#include "shellitem.h"

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
    m_grabWindow->resize(1, 1);
    m_grabWindow->create();
    m_grabWindow->show();

    wl_surface *grabSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_grabWindow));
    desktop_shell_set_grab_surface(m_shell, grabSurface);

    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));

    QScreen *screen = QGuiApplication::screens().first();
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setAlphaBufferSize(8);
    format.setStencilBufferSize(2);

    m_engine = new QQmlEngine(this);
    m_engine->rootContext()->setContextProperty("Client", this);
    m_engine->rootContext()->setContextProperty("ProcessLauncher", m_launcher);

    m_component = new QQmlComponent(m_engine, this);
    m_component->loadUrl(path + "desktop.qml");
    if (!m_component->isReady())
        qFatal(qPrintable(m_component->errorString()));

    QObject *rootObject = m_component->create();
    if (!rootObject)
        qFatal("Couldn't create component from Shell.qml!");


    const QObjectList objects = rootObject->children();
    for (int i = 0; i < objects.size(); i++) {
        ShellItem *window = qobject_cast<ShellItem *>(objects.at(i));
        if (!window)
            continue;

        window->setFormat(format);
        window->setFlags(Qt::BypassWindowManagerHint);
        window->setScreen(screen);
        window->setColor(Qt::transparent);
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

    QCursor qcursor;
    switch (cursor) {
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

    object->m_grabWindow->setCursor(qcursor);
}

const desktop_shell_listener Client::s_shellListener = {
    Client::configure,
    Client::handlePrepareLockSurface,
    Client::handleGrabCursor
};

#include "client.moc"
