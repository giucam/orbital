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
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/input.h>

#include <QDebug>
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>
#include <QProcess>
#include <QObjectCleanupHandler>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QFile>

#include <compositor.h>

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
#include "global.h"

namespace Orbital {

static int s_signalsFd[2];
static int s_forceExit = 0;

static int log(const char *fmt, va_list ap)
{
    return vprintf(fmt, ap);
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
    wl_listener outputCreatedSignal;
    wl_listener outputMovedSignal;
    wl_listener sessionSignal;
    wl_listener seatCreatedSignal;
    Compositor *compositor;
};

Compositor::Compositor(Backend *backend)
          : QObject()
          , m_display(wl_display_create())
          , m_compositor(nullptr)
          , m_listener(new Listener)
          , m_backend(backend)
          , m_shell(nullptr)
          , m_bindingsCleanupHandler(new QObjectCleanupHandler)
{
    connect(&m_fakeRepaintLoopTimer, &QTimer::timeout, this, &Compositor::fakeRepaint);

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signalsFd)) {
        qFatal("Couldn't create signals socketpair");
    }

    m_signalsNotifier = new QSocketNotifier(s_signalsFd[1], QSocketNotifier::Read, this);
    connect(m_signalsNotifier, &QSocketNotifier::activated, this, &Compositor::handleSignal);

    struct sigaction sigint, sigterm;

    auto handler = [](int) {
        if (s_forceExit) {
            exit(1);
        }

        char a = 1;
        ::write(s_signalsFd[0], &a, sizeof(a));
        s_forceExit = 1;
    };

    sigint.sa_handler = handler;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigint.sa_flags |= SA_RESTART;
    sigterm.sa_handler = handler;
    sigemptyset(&sigterm.sa_mask);
    sigterm.sa_flags = 0;
    sigterm.sa_flags |= SA_RESTART;

    sigaction(SIGINT, &sigint, 0);
    sigaction(SIGTERM, &sigterm, 0);

    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString configFile = path + "/orbital/orbital.conf";

    QFile file(configFile);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        file.close();
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    m_config = doc.object();
}

Compositor::~Compositor()
{
    delete m_shell;
    qDeleteAll(m_outputs);
    delete m_rootLayer;
    delete m_overlayLayer;
    delete m_panelsLayer;
    delete m_stickyLayer;
    delete m_appsLayer;
    delete m_backgroundLayer;
    delete m_bindingsCleanupHandler;

    if (m_compositor)
        weston_compositor_destroy(m_compositor);
    delete m_listener;
    delete m_backend;

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

static const weston_pointer_grab_interface defaultPointerGrab = {
    [](weston_pointer_grab *grab) {
        weston_pointer *p = grab->pointer;
        Seat::fromSeat(p->seat)->pointer()->defaultGrabFocus();
    },
    [](weston_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabMotion(time, wl_fixed_to_double(x), wl_fixed_to_double(y));
    },
    [](weston_pointer_grab *grab, uint32_t time, uint32_t btn, uint32_t state) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabButton(time, btn, state);
    },
    [](weston_pointer_grab *grab) {}
};

bool Compositor::init(const QString &socketName)
{
    weston_log_set_handler(log, log);

    verify_xdg_runtime_dir();

    m_compositor = static_cast<weston_compositor *>(malloc(sizeof *m_compositor));
    memset(m_compositor, 0, sizeof(*m_compositor));

    m_compositor->wl_display = m_display;
    m_compositor->idle_time = 300;

    QJsonObject kbdConfig = m_config["Compositor"].toObject()["Keyboard"].toObject();
    QString keylayout = kbdConfig["Layout"].toString();
    QString keyoptions = kbdConfig["Options"].toString();

    xkb_rule_names xkb = { nullptr, nullptr,
                           keylayout.isEmpty() ? nullptr : strdup(qPrintable(keylayout)),
                           nullptr,
                           keyoptions.isEmpty() ? nullptr : strdup(qPrintable(keyoptions)) };

    if (weston_compositor_init(m_compositor) < 0 || weston_compositor_xkb_init(m_compositor, &xkb) < 0)
        return false;

    m_rootLayer = new Layer(&m_compositor->cursor_layer);
    m_lockLayer = new Layer(m_rootLayer);
    m_overlayLayer = new Layer(m_rootLayer);
    m_fullscreenLayer = new Layer(m_rootLayer);
    m_panelsLayer = new Layer(m_rootLayer);
    m_stickyLayer = new Layer(m_rootLayer);
    m_appsLayer = new Layer(m_rootLayer);
    m_backgroundLayer = new Layer(m_rootLayer);
    m_baseBackgroundLayer = new Layer(m_rootLayer);
    m_minimizedLayer = new Layer(m_rootLayer);
    m_minimizedLayer->setMask(0, 0, 0, 0);

    m_compositor->kb_repeat_rate = 40;
    m_compositor->kb_repeat_delay = 400;
    m_compositor->terminate = terminate;

    m_listener->compositor = this;
    m_listener->listener.notify = compositorDestroyed;
    wl_signal_add(&m_compositor->destroy_signal, &m_listener->listener);
    m_listener->outputMovedSignal.notify = [](wl_listener *l, void *data) {
        if (Output *o = Output::fromOutput(static_cast<weston_output *>(data))) {
            emit o->moved();
        }
    };
    wl_signal_add(&m_compositor->output_moved_signal, &m_listener->outputMovedSignal);
    m_listener->outputCreatedSignal.notify = [](wl_listener *l, void *data) {
        Listener *listener = container_of(l, Listener, outputCreatedSignal);
        listener->compositor->newOutput(static_cast<weston_output *>(data));
    };
    wl_signal_add(&m_compositor->output_created_signal, &m_listener->outputCreatedSignal);
    m_listener->sessionSignal.notify = [](wl_listener *l, void *data) {
        Listener *listener = container_of(l, Listener, sessionSignal);
        emit listener->compositor->sessionActivated(listener->compositor->m_compositor->session_active);
    };
    wl_signal_add(&m_compositor->session_signal, &m_listener->sessionSignal);
    m_listener->seatCreatedSignal.notify = [](wl_listener *l, void *data)
    {
        Listener *listener = container_of(l, Listener, seatCreatedSignal);
        weston_seat *s = static_cast<weston_seat *>(data);
        emit listener->compositor->seatCreated(Seat::fromSeat(s));
    };
    wl_signal_add(&m_compositor->seat_created_signal, &m_listener->seatCreatedSignal);
//     text_backend_init(m_compositor, "");

    if (!m_backend->init(m_compositor)) {
        weston_compositor_shutdown(m_compositor);
        return false;
    }

    if (m_compositor->launcher) {
        for (int i = KEY_F1; i < KEY_F12; ++i) {
            KeyBinding *b = createKeyBinding(i, KeyboardModifiers::Ctrl | KeyboardModifiers::Alt);
            connect(b, &KeyBinding::triggered, [this, i]() {
                int vt = i - KEY_F1 + 1;
                if (weston_launcher_get_vt(m_compositor->launcher) != vt) {
                    m_shell->lock([this, vt]() {
                        weston_launcher_activate_vt(m_compositor->launcher, vt);
                    });
                }
            });
        }
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
    int fd = wl_event_loop_get_fd(m_loop);

    QSocketNotifier *sockNot = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(sockNot, &QSocketNotifier::activated, this, &Compositor::processEvents);
    QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
    connect(dispatcher, &QAbstractEventDispatcher::awake, this, &Compositor::processIdle);

    weston_compositor_add_key_binding(m_compositor, KEY_BACKSPACE,
                          (weston_keyboard_modifier)(MODIFIER_CTRL | MODIFIER_ALT),
                          terminate_binding, this);
    weston_install_debug_key_binding(m_compositor, MODIFIER_SUPER);

    weston_compositor_set_default_pointer_grab(m_compositor, &defaultPointerGrab);

    m_shell = new Shell(this);
    Workspace *ws = m_shell->createWorkspace();
    for (Output *o: m_outputs) {
        m_shell->pager()->activate(ws, o);
    }

    connect(this, &Compositor::sessionActivated, [this](bool a) {
        if (!a) {
            m_fakeRepaintLoopTimer.start(100);
        } else {
            m_fakeRepaintLoopTimer.stop();
        }
    });

    return true;
}

void Compositor::newOutput(weston_output *output)
{
    QJsonObject outputs = m_config["Compositor"].toObject()["Outputs"].toObject();
    QJsonObject cfg = outputs[output->name].toObject();
    int x = output->x;
    int y = output->y;
    if (cfg.contains("x")) {
        x = cfg["x"].toInt();
    }
    if (cfg.contains("y")) {
        y = cfg["y"].toInt();
    }
    weston_output_move(output, x, y);

    Output *o = new Output(output);
    connect(o, &QObject::destroyed, this, &Compositor::outputDestroyed);
    m_outputs << o;

    emit outputCreated(o);
}

//XXX FIXME This comes from compositor.c, it should not stay here!!
struct weston_frame_callback {
    struct wl_resource *resource;
    struct wl_list link;
};

void Compositor::fakeRepaint()
{
    wl_list frame_callback_list;
    wl_list_init(&frame_callback_list);
    weston_view *view;
    wl_list_for_each(view, &m_compositor->view_list, link) {
        wl_list_insert_list(&frame_callback_list, &view->surface->frame_callback_list);
        wl_list_init(&view->surface->frame_callback_list);
    }

    weston_frame_callback *cb, *cnext;
    wl_list_for_each_safe(cb, cnext, &frame_callback_list, link) {
        wl_callback_send_done(cb->resource, 0);
        wl_resource_destroy(cb->resource);
    }
}

void Compositor::quit()
{
    qDebug() << "Orbital exiting...";
    QCoreApplication::quit();
}

void Compositor::processEvents()
{
    wl_event_loop_dispatch(m_loop, 0);
    wl_display_flush_clients(m_display);
}

void Compositor::processIdle()
{
    wl_event_loop_dispatch_idle(m_loop);
}

Shell *Compositor::shell() const
{
    return m_shell;
}

Layer *Compositor::lockLayer() const
{
    return m_lockLayer;
}

Layer *Compositor::overlayLayer() const
{
    return m_overlayLayer;
}

Layer *Compositor::fullscreenLayer() const
{
    return m_fullscreenLayer;
}

Layer *Compositor::panelsLayer() const
{
    return m_panelsLayer;
}

Layer *Compositor::stickyLayer() const
{
    return m_stickyLayer;
}

Layer *Compositor::appsLayer() const
{
    return m_appsLayer;
}

Layer *Compositor::backgroundLayer() const
{
    return m_backgroundLayer;
}

Layer *Compositor::baseBackgroundLayer() const
{
    return m_baseBackgroundLayer;
}

Layer *Compositor::minimizedLayer() const
{
    return m_minimizedLayer;
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

uint32_t Compositor::nextSerial() const
{
    return wl_display_next_serial(m_compositor->wl_display);
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

void Compositor::kill(Surface *surface)
{
    wl_signal_emit(&m_compositor->kill_signal, surface->surface());

    wl_client *client = surface->client();
    pid_t pid;
    wl_client_get_credentials(client, &pid, NULL, NULL);

    if (pid != getpid()) {
        ::kill(pid, SIGKILL);
    }
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

AxisBinding *Compositor::createAxisBinding(PointerAxis axis, KeyboardModifiers modifier)
{
    AxisBinding *b = new AxisBinding(m_compositor, axis, modifier);
    m_bindingsCleanupHandler->add(b);
    return b;
}

HotSpotBinding *Compositor::createHotSpotBinding(PointerHotSpot hs)
{
    HotSpotBinding *b = new HotSpotBinding(hs);
    m_bindingsCleanupHandler->add(b);
    m_hotSpotBindings.insert((int)hs, b);
    return b;
}

void Compositor::handleHotSpot(Seat *seat, uint32_t time, PointerHotSpot hs)
{
    for (auto i = m_hotSpotBindings.find((int)hs); i != m_hotSpotBindings.end(); ++i) {
        emit (*i)->triggered(seat, time, hs);
    }
}

Compositor *Compositor::fromCompositor(weston_compositor *c)
{
    wl_listener *listener = wl_signal_get(&c->destroy_signal, compositorDestroyed);
    Q_ASSERT(listener);
    return reinterpret_cast<Listener *>(listener)->compositor;
}

void Compositor::outputDestroyed()
{
    Output *o = static_cast<Output *>(sender());
    m_outputs.removeOne(o);
    emit outputRemoved(o);
}

void Compositor::handleSignal()
{
    char tmp;
    ::read(s_signalsFd[1], &tmp, sizeof(tmp));

    quit();
}



// -- ChildProcess

struct ChildProcess::Listener
{
    wl_listener listener;
    ChildProcess *parent;
};

ChildProcess::ChildProcess(wl_display *dpy, const QString &program)
            : QObject()
            , m_display(dpy)
            , m_program(program)
            , m_client(nullptr)
            , m_autoRestart(false)
            , m_listener(new Listener)
            , m_deathCount(0)
{
    m_listener->parent = this;
    m_listener->listener.notify = [](wl_listener *l, void *data) {
        ChildProcess *p = reinterpret_cast<Listener *>(l)->parent;
        p->finished();
    };
    wl_list_init(&m_listener->listener.link);
}

ChildProcess::~ChildProcess()
{
    wl_list_remove(&m_listener->listener.link);
    delete m_listener;
    if (m_client) {
        wl_client_destroy(m_client);
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

    class Process : public QProcess
    {
    public:
        Process(int fd) : QProcess(), socket(fd) {}
        void setupChildProcess() override
        {
            int fd = dup(socket);
            setenv("WAYLAND_SOCKET", qPrintable(QString::number(fd)), 1);
        }

        int socket;
    };

    Process *process = new Process(sv[1]);
    connect(process, (void (QProcess::*)(QProcess::ProcessError))&QProcess::error, [this](QProcess::ProcessError err) {
        qDebug("%s: error %d\n", qPrintable(m_program), (int)err);
    });
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->start(m_program);
    connect(process, &QProcess::started, [sv]() { close(sv[1]); });
    connect(process, (void (QProcess::*)(int))&QProcess::finished, process, &QObject::deleteLater);

    m_client = wl_client_create(m_display, sv[0]);
    if (!m_client) {
        close(sv[0]);
        qDebug("wl_client_create failed while launching '%s'.", qPrintable(m_program));
        return;
    }

    wl_client_add_destroy_listener(m_client, &m_listener->listener);

    if (m_deathCount == 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        m_startTime = now.tv_sec;
    }
}

void ChildProcess::restart()
{
    if (m_client) {
        wl_list_remove(&m_listener->listener.link);
        wl_list_init(&m_listener->listener.link);
        wl_client_destroy(m_client);
        m_client = nullptr;
    }
    start();
}

void ChildProcess::finished()
{
    m_client = nullptr;
    wl_list_remove(&m_listener->listener.link);
    wl_list_init(&m_listener->listener.link);

    if (m_autoRestart) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        if (now.tv_sec - m_startTime < 30) {
            m_deathCount++;
        } else {
            m_deathCount = 0;
        }

        if (m_deathCount > 4) {
            qDebug("%s exited too often too fast. Giving up.", qPrintable(m_program));
            m_deathCount = 0;
            emit givingUp();
        } else {
            qDebug("%s exited. Restarting it...", qPrintable(m_program));
            start();
        }
    } else {
        qDebug("%s exited.", qPrintable(m_program));
    }
}

}
