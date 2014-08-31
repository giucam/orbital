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

#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/input.h>

#include <QDebug>
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>
#include <QProcess>
#include <QObjectCleanupHandler>

#include <weston/compositor.h>

#include "compositor.h"
#include "backend.h"
#include "shell.h"
#include "view.h"
#include "layer.h"
#include "workspace.h"
#include "output.h"
#include "dummysurface.h"
#include "seat.h"
#include "binding.h"
#include "pager.h"

namespace Orbital {

static int log(const char *fmt, va_list ap)
{
    return vprintf(fmt, ap);
    return 0;
}

static void terminate(weston_compositor *c)
{
    Compositor::fromCompositor(c)->quit();
}

static void terminate_binding(weston_seat *seat, uint32_t time, uint32_t key, void *data)
{
    Compositor *c = static_cast<Compositor *>(data);
    c->quit();
}

struct Listener {
    wl_listener listener;
    Compositor *compositor;
};

Compositor::Compositor(Backend *backend)
          : QObject()
          , m_display(wl_display_create())
          , m_compositor(nullptr)
          , m_listener(new Listener)
          , m_backend(backend)
          , m_bindingsCleanupHandler(new QObjectCleanupHandler)
{
    m_timer.setInterval(0);
    connect(&m_timer, &QTimer::timeout, this, &Compositor::processEvents);
    m_timer.start();
}

Compositor::~Compositor()
{
    delete m_bindingsCleanupHandler;
    delete m_shell;
    wl_list_remove(&m_listener->listener.link);
    delete m_listener;
    delete m_backend;
    delete m_rootLayer;
    delete m_overlayLayer;
    delete m_panelsLayer;
    delete m_appsLayer;
    delete m_backgroundLayer;

    if (m_compositor)
        weston_compositor_destroy(m_compositor);

    wl_display_destroy(m_display);
}

static void compositorDestroyed(wl_listener *listener, void *data)
{
}

static const char xdg_error_message[] =
    "fatal: environment variable XDG_RUNTIME_DIR is not set.\n";

static const char xdg_wrong_message[] =
    "fatal: environment variable XDG_RUNTIME_DIR\n"
    "is set to \"%s\", which is not a directory.\n";

static const char xdg_wrong_mode_message[] =
    "warning: XDG_RUNTIME_DIR \"%s\" is not configured\n"
    "correctly.  Unix access mode must be 0700 (current mode is %o),\n"
    "and must be owned by the user (current owner is UID %d).\n";

static const char xdg_detail_message[] =
    "Refer to your distribution on how to get it, or\n"
    "http://www.freedesktop.org/wiki/Specifications/basedir-spec\n"
    "on how to implement it.\n";

static void
verify_xdg_runtime_dir(void)
{
    char *dir = getenv("XDG_RUNTIME_DIR");
    struct stat s;

    if (!dir) {
        weston_log(xdg_error_message);
        weston_log_continue(xdg_detail_message);
        exit(EXIT_FAILURE);
    }

    if (stat(dir, &s) || !S_ISDIR(s.st_mode)) {
        weston_log(xdg_wrong_message, dir);
        weston_log_continue(xdg_detail_message);
        exit(EXIT_FAILURE);
    }

    if ((s.st_mode & 0777) != 0700 || s.st_uid != getuid()) {
        weston_log(xdg_wrong_mode_message,
               dir, s.st_mode & 0777, s.st_uid);
        weston_log_continue(xdg_detail_message);
    }
}


bool Compositor::init(const QString &socketName)
{
    weston_log_set_handler(log, log);

    verify_xdg_runtime_dir();

    m_compositor = weston_compositor_create(m_display);

    xkb_rule_names xkb = { NULL, NULL, strdup("it"), NULL, NULL };

    if (weston_compositor_xkb_init(m_compositor, &xkb) < 0)
        return false;

    m_rootLayer = new Layer(&m_compositor->cursor_layer);
    m_overlayLayer = new Layer(m_rootLayer);
    m_panelsLayer = new Layer(m_overlayLayer);
    m_appsLayer = new Layer(m_panelsLayer);
    m_backgroundLayer = new Layer(m_appsLayer);

    m_compositor->kb_repeat_rate = 40;
    m_compositor->kb_repeat_delay = 400;
    m_compositor->terminate = terminate;

    m_listener->listener.notify = compositorDestroyed;
    m_listener->compositor = this;
    wl_signal_add(&m_compositor->destroy_signal, &m_listener->listener);
//     text_backend_init(m_compositor, "");

    if (!m_backend->init(m_compositor)) {
        weston_compositor_shutdown(m_compositor);
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

    weston_compositor_add_key_binding(m_compositor, KEY_BACKSPACE,
                          (weston_keyboard_modifier)(MODIFIER_CTRL | MODIFIER_ALT),
                          terminate_binding, this);

    weston_output *o;
    wl_list_for_each(o, &m_compositor->output_list, link) {
        m_outputs << new Output(o);
    }

    m_shell = new Shell(this);
    Workspace *ws = m_shell->createWorkspace();
    for (Output *o: m_outputs) {
        m_shell->pager()->activate(ws, o);
    }

    return true;
}

void Compositor::quit()
{
    qDebug() << "Orbital exiting...";
    QCoreApplication::quit();
}

void Compositor::processEvents()
{
    wl_display_flush_clients(m_display);
    wl_event_loop_dispatch(m_loop, -1);
}

Shell *Compositor::shell() const
{
    return m_shell;
}

Layer *Compositor::rootLayer() const
{
    return m_rootLayer;
}

Layer *Compositor::overlayLayer() const
{
    return m_overlayLayer;
}

Layer *Compositor::panelsLayer() const
{
    return m_panelsLayer;
}

Layer *Compositor::appsLayer() const
{
    return m_appsLayer;
}

Layer *Compositor::backgroundLayer() const
{
    return m_backgroundLayer;
}

QList<Output *> Compositor::outputs() const
{
    return m_outputs;
}

QList<Seat *> Compositor::seats() const
{
    QList<Seat *> seats;
    weston_seat *seat;
    wl_list_for_each(seat, &m_compositor->seat_list, link) {
        seats << Seat::fromSeat(seat);
    }
    return seats;
}

DummySurface *Compositor::createDummySurface(int w, int h)
{
    return new DummySurface(weston_surface_create(m_compositor), w, h);
}

uint32_t Compositor::serial() const
{
    return wl_display_get_serial(m_compositor->wl_display);
}

View *Compositor::pickView(double x, double y, double *vx, double *vy) const
{
    wl_fixed_t fx = wl_fixed_from_double(x);
    wl_fixed_t fy = wl_fixed_from_double(y);
    wl_fixed_t fvx, fvy;
    weston_view *v = weston_compositor_pick_view(m_compositor, fx, fy, &fvx, &fvy);

    if (vx)
        *vx = wl_fixed_to_double(fvx);
    if (vy)
        *vy = wl_fixed_to_double(fvy);

    return View::fromView(v);
}

ChildProcess *Compositor::launchProcess(const QString &path)
{
    qDebug("Launching '%s'...", qPrintable(path));
    ChildProcess *p = new ChildProcess(m_compositor->wl_display, path);
    p->start();
    return p;
}

ButtonBinding *Compositor::createButtonBinding(PointerButton button, KeyboardModifiers modifiers)
{
    ButtonBinding *b = new ButtonBinding(m_compositor, button, modifiers, this);
    m_bindingsCleanupHandler->add(b);
    return b;
}

KeyBinding *Compositor::createKeyBinding(uint32_t key, KeyboardModifiers modifiers)
{
    KeyBinding *b = new KeyBinding(m_compositor, key, modifiers, this);
    m_bindingsCleanupHandler->add(b);
    return b;
}

Compositor *Compositor::fromCompositor(weston_compositor *c)
{
    wl_listener *listener = wl_signal_get(&c->destroy_signal, compositorDestroyed);
    Q_ASSERT(listener);
    return reinterpret_cast<Listener *>(listener)->compositor;
}



// -- ChildProcess

ChildProcess::ChildProcess(wl_display *dpy, const QString &program)
            : QObject()
            , m_display(dpy)
            , m_program(program)
            , m_process(nullptr)
            , m_client(nullptr)
            , m_autoRestart(false)
{
}

ChildProcess::~ChildProcess()
{
    if (m_process) {
        delete m_process;
        close(m_socketFd);
    }
}

void ChildProcess::setAutoRestart(bool enabled)
{
    m_autoRestart = enabled;
}

wl_client *ChildProcess::client() const
{
    return m_client;
}

void ChildProcess::start()
{
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    int fd = dup(sv[1]);
    env.insert("WAYLAND_SOCKET", QString::number(fd));
    env.insert("QT_QPA_PLATFORM", "wayland");
    close(sv[1]);

    m_client = wl_client_create(m_display, sv[0]);
    if (!m_client) {
        close(sv[0]);
        qDebug("wl_client_create failed while launching '%s'.", qPrintable(m_program));
        return;
    }

    m_process = new QProcess;
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_process->setProcessEnvironment(env);
    m_process->start(m_program);
    connect(m_process, (void (QProcess::*)(int))&QProcess::finished, this, &ChildProcess::finished);
    m_socketFd = sv[0];
}

void ChildProcess::finished(int code)
{
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
        close(m_socketFd);
    }

    if (m_autoRestart) {
        qDebug("%s exited with exit code '%d'. Restarting it...", qPrintable(m_program), code);
        start();
    } else {
        qDebug("%s exited with exit code '%d'.", qPrintable(m_program), code);
    }
}

}
