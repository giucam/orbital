
#include <QDebug>
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>

#include <weston/compositor.h>

#include "compositor.h"
#include "backend.h"
#include "shell.h"
#include "view.h"
#include "layer.h"
#include "workspace.h"
#include "output.h"
#include "dummysurface.h"

namespace Orbital {

static int log(const char *fmt, va_list ap)
{
//     return vprintf(fmt, ap);
    return 0;
}

static void terminate(weston_compositor *)
{
    QCoreApplication::quit();
}

Compositor::Compositor(Backend *backend)
          : QObject()
          , m_display(wl_display_create())
          , m_compositor(nullptr)
          , m_backend(backend)
{
    m_timer.setInterval(0);
    connect(&m_timer, &QTimer::timeout, this, &Compositor::processEvents);
    m_timer.start();
}

Compositor::~Compositor()
{
    if (m_compositor)
        weston_compositor_destroy(m_compositor);

    wl_display_destroy(m_display);

    delete m_backend;
    delete m_rootLayer;
}

bool Compositor::init(const QString &socketName)
{
    weston_log_set_handler(log, log);

    m_compositor = weston_compositor_create(m_display);

    xkb_rule_names xkb = { NULL, NULL, strdup("it"), NULL, NULL };

    if (weston_compositor_xkb_init(m_compositor, &xkb) < 0)
        return false;

    m_rootLayer = new Layer(&m_compositor->cursor_layer);
    m_compositor->kb_repeat_rate = 40;
    m_compositor->kb_repeat_delay = 400;
    m_compositor->terminate = terminate;
//     text_backend_init(m_compositor, "");

    if (!m_backend->init(m_compositor)) {
        return false;
    }

    const char *socket = qPrintable(socketName);
    if (!socketName.isNull()) {
        if (wl_display_add_socket(m_display, qPrintable(socket))) {
            weston_log("fatal: failed to add socket: %m\n");
            return false;
        }
    } else {
        socket = wl_display_add_socket_auto(m_display);
        if (!socket) {
            weston_log("fatal: failed to add socket: %m\n");
            return false;
        }
    }
    weston_log("Using '%s'\n", socket);

    setenv("WAYLAND_DISPLAY", socket, 1);

    weston_compositor_wake(m_compositor);

    m_loop = wl_display_get_event_loop(m_display);
//     int fd = wl_event_loop_get_fd(m_loop);

//     QSocketNotifier *sockNot = new QSocketNotifier(fd, QSocketNotifier::Read, this);
//     connect(sockNot, &QSocketNotifier::activated, this, &Compositor::processEvents);
//     QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
//     connect(dispatcher, &QAbstractEventDispatcher::aboutToBlock, this, &Compositor::processEvents);

    weston_output *o;
    wl_list_for_each(o, &m_compositor->output_list, link) {
        m_outputs << new Output(o);
    }

    m_shell = new Shell(this);
    Workspace *ws = new Workspace(m_shell);
    m_shell->addWorkspace(ws);
    for (int i = 0; i < 4; ++i) {
        m_shell->addWorkspace(new Workspace(m_shell));
    }

    for (Output *o: m_outputs) {
        o->viewWorkspace(ws);
    }

    return true;
}

void Compositor::processEvents()
{
    wl_display_flush_clients(m_display);
    wl_event_loop_dispatch(m_loop, -1);
}

Layer *Compositor::rootLayer() const
{
    return m_rootLayer;
}

QList<Output *> Compositor::outputs() const
{
    return m_outputs;
}

DummySurface *Compositor::createDummySurface(int w, int h)
{
    return new DummySurface(weston_surface_create(m_compositor), w, h);
}

}
