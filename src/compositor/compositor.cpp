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
#include <sys/resource.h>
#include <linux/input.h>

#include <QDebug>
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
#include "authorizer.h"
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "debug.h"

namespace Orbital {

static const int WATCHDOG_TIMEOUT = 20;

static int s_signalsFd[2];
static int s_forceExit = 0;

volatile sig_atomic_t alarmFired = 0;

static wl_event_loop *s_event_loop;

Timer::Timer()
     : m_func(nullptr)
     , m_timerSource(nullptr)
     , m_idleSource(nullptr)
     , m_interval(-1)
     , m_repeat(true)
{
}

Timer::~Timer()
{
    if (!s_event_loop) {
        return;
    }

    if (m_timerSource) {
        wl_event_source_remove(m_timerSource);
    }
    if (m_idleSource) {
        wl_event_source_remove(m_idleSource);
    }
}

void Timer::setRepeat(bool repeat)
{
    m_repeat = repeat;
}

void Timer::setTimeoutHandler(const std::function<void ()> &func)
{
    m_func = func;
}

void Timer::start(int msecs, const std::function<void ()> &func)
{
    m_func = func;
    start(msecs);
}

void Timer::start(int msecs)
{
    m_interval = msecs;

    if (!m_timerSource) {
        m_timerSource = wl_event_loop_add_timer(s_event_loop, [](void *data) {
            Timer *t = static_cast<Timer *>(data);
            t->timeout();
            return 0;
        }, this);
    }
    rearm();
}

void Timer::stop()
{
    m_interval = -1;
    rearm();
}

void Timer::timeout()
{
    m_func();
    if (m_repeat) {
        rearm();
    }
}

void Timer::rearm()
{
    if (m_interval == 0 && !m_idleSource) {
        m_idleSource = wl_event_loop_add_idle(s_event_loop, [](void *data) {
            Timer *t = static_cast<Timer *>(data);
            t->m_idleSource = nullptr;
            t->timeout();
        }, this);
    } else if (m_interval != 0 && m_idleSource) {
        wl_event_source_remove(m_idleSource);
        m_idleSource = nullptr;
    }

    wl_event_source_timer_update(m_timerSource, m_interval < 0 ? 0 : m_interval);
}

void Timer::singleShot(int msecs, const std::function<void ()> &func)
{
    if (msecs < 0) {
        return;
    }

    struct SingleShot {
        std::function<void ()> func;
        wl_event_source *source;
    };

    SingleShot *ss = new SingleShot{ func, nullptr };

    if (msecs == 0) {
        wl_event_loop_add_idle(s_event_loop, [](void *data) {
            auto *ss = static_cast<SingleShot *>(data);
            ss->func();
            delete ss;
        }, ss);
    } else {
        ss->source = wl_event_loop_add_timer(s_event_loop, [](void *data) {
            auto *ss = static_cast<SingleShot *>(data);
            ss->func();
            wl_event_source_remove(ss->source);
            delete ss;
            return 0;
        }, ss);
        wl_event_source_timer_update(ss->source, msecs);
    }
}




static int log(const char *fmt, va_list ap)
{
    return vprintf(fmt, ap);
}

static void terminate(weston_compositor *c)
{
    Compositor::fromCompositor(c)->quit();
}

static void terminate_binding(weston_keyboard *keyb, uint32_t time, uint32_t key, void *data)
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
          , m_authorizer(nullptr)
{
    m_fakeRepaintLoopTimer.setTimeoutHandler([this]() {
        fakeRepaint();
    });

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signalsFd)) {
        qFatal("Couldn't create signals socketpair");
    }

    s_event_loop = wl_display_get_event_loop(m_display);
    wl_event_loop_add_fd(s_event_loop, s_signalsFd[1], WL_EVENT_READABLE, [](int fd, uint32_t mask, void *data) {
        char tmp;
        ::read(fd, &tmp, sizeof(tmp));

        static_cast<Compositor *>(data)->quit();
        return 0;
    }, this);

    struct sigaction sigint, sigterm, sigalrm;

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

    sigalrm.sa_handler = [](int) {
        if (++alarmFired > 1) {
            abort();
        }
        alarm(WATCHDOG_TIMEOUT);
    };
    sigemptyset(&sigalrm.sa_mask);
    sigalrm.sa_flags = 0;
    sigalrm.sa_flags |= SA_RESTART;

    sigaction(SIGINT, &sigint, 0);
    sigaction(SIGTERM, &sigterm, 0);
    sigaction(SIGALRM, &sigalrm, 0);

    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString configFile = path + QLatin1String("/orbital/orbital.conf");

    QFile file(configFile);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        file.close();
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    m_config = doc.object();

    alarm(WATCHDOG_TIMEOUT);
    m_watchdogTimer.start(10000, [this]() {        alarm(WATCHDOG_TIMEOUT);        alarmFired = 0;    });
}

Compositor::~Compositor()
{
    for (auto seat: seats()) {
        delete seat;
    }
    delete m_shell;
    delete m_authorizer;
    qDeleteAll(m_outputs);
    delete m_bindingsCleanupHandler;

    // we must destroy the layers before destroying the compositor, otherwise ~Layer() will
    // call wl_list_remove with an invalid list
    m_layers.clear();

    if (m_compositor)
        weston_compositor_destroy(m_compositor);
    delete m_listener;
    delete m_backend;

    // we reset s_event_loop so that ~Timer() won't delete the event sources, since they are automatically
    // freed when the display is destroyed
    s_event_loop = nullptr;
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
    [](weston_pointer_grab *grab, uint32_t time, weston_pointer_motion_event *event) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabMotion(time, Pointer::MotionEvent(event));
    },
    [](weston_pointer_grab *grab, uint32_t time, uint32_t btn, uint32_t state) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabButton(time, btn, state);
    },
    [](weston_pointer_grab *grab, uint32_t time, weston_pointer_axis_event *event) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabAxis(time, Pointer::AxisEvent(event));
    },
    [](weston_pointer_grab *grab, uint32_t source) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabAxisSource(source);
    },
    [](weston_pointer_grab *grab) {
        Seat *seat = Seat::fromSeat(grab->pointer->seat);
        seat->pointer()->defaultGrabFrame();
    },
    [](weston_pointer_grab *grab) {}
};

bool Compositor::init(StringView socketName)
{
    weston_log_set_handler(log, log);

    verify_xdg_runtime_dir();

    m_compositor = weston_compositor_create(m_display, this);
    if (!m_compositor) {
        return false;
    }

    m_compositor->idle_time = 300;

    QJsonObject kbdConfig = m_config[QStringLiteral("Compositor")].toObject()[QStringLiteral("Keyboard")].toObject();
    const QByteArray keylayout = kbdConfig[QStringLiteral("Layout")].toString().toUtf8();
    const QByteArray keyoptions = kbdConfig[QStringLiteral("Options")].toString().toUtf8();
    const QByteArray keyvariant = kbdConfig[QStringLiteral("Variant")].toString().toUtf8();

    m_defaultKeymap = Keymap(keylayout.isEmpty() ? Maybe<StringView>() : StringView(keylayout),
                             keyoptions.isEmpty() ? Maybe<StringView>() : StringView(keyoptions),
                             keyvariant.isEmpty() ? Maybe<StringView>() : StringView(keyvariant));

    xkb_rule_names xkb = { nullptr, nullptr,
                           keylayout.isEmpty() ? nullptr : strdup(keylayout.data()),
                           keyvariant.isEmpty() ? nullptr : strdup(keyvariant.data()),
                           keyoptions.isEmpty() ? nullptr : strdup(keyoptions.data()) };

    if (weston_compositor_set_xkb_rule_names(m_compositor, &xkb) < 0)
        return false;

    for (int i = 0; i <= (int)Layer::Minimized; ++i) {
        m_layers.emplace_back(&m_compositor->cursor_layer);
    }
    layer(Layer::Minimized)->setMask(0, 0, 0, 0);

    m_compositor->kb_repeat_rate = 40;
    m_compositor->kb_repeat_delay = 400;
    m_compositor->exit = terminate;
    m_compositor->vt_switching = true;

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
        Listener *listener = wl_container_of(l, (Listener *)nullptr, outputCreatedSignal);
        listener->compositor->newOutput(static_cast<weston_output *>(data));
    };
    wl_signal_add(&m_compositor->output_created_signal, &m_listener->outputCreatedSignal);
    m_listener->sessionSignal.notify = [](wl_listener *l, void *data) {
        Listener *listener = wl_container_of(l, (Listener *)nullptr, sessionSignal);
        emit listener->compositor->sessionActivated(listener->compositor->m_compositor->session_active);
    };
    wl_signal_add(&m_compositor->session_signal, &m_listener->sessionSignal);
    m_listener->seatCreatedSignal.notify = [](wl_listener *l, void *data)
    {
        Listener *listener = wl_container_of(l, (Listener *)nullptr, seatCreatedSignal);
        weston_seat *s = static_cast<weston_seat *>(data);
        emit listener->compositor->seatCreated(Seat::fromSeat(s));
    };
    wl_signal_add(&m_compositor->seat_created_signal, &m_listener->seatCreatedSignal);
//     text_backend_init(m_compositor, "");

    if (!m_backend->init(m_compositor)) {
        return false;
    }

    const char *socket = nullptr;
    std::string socketStr;
    if (!socketName.isEmpty()) {
        socketStr = socketName.toStdString();
        socket = socketStr.data();

        if (wl_display_add_socket(m_display, socket)) {
            fmt::print("fatal: failed to add socket: {}\n", strerror(errno));
            return false;
        }
    } else {
        socket = wl_display_add_socket_auto(m_display);
        if (!socket) {
            fmt::print("fatal: failed to add socket: {}\n", strerror(errno));
            return false;
        }
    }
    fmt::print("Using '{}'\n", socket);

    setenv("WAYLAND_DISPLAY", socket, 1);

    weston_compositor_wake(m_compositor);

    weston_compositor_set_default_pointer_grab(m_compositor, &defaultPointerGrab);

    m_authorizer = new Authorizer(this);
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
    QJsonObject outputs = m_config[QStringLiteral("Compositor")].toObject()[QStringLiteral("Outputs")].toObject();
    QJsonObject cfg = outputs[QString::fromUtf8(output->name)].toObject();
    int x = output->x;
    int y = output->y;
    if (cfg.contains(QStringLiteral("x"))) {
        x = cfg[QStringLiteral("x")].toInt();
    }
    if (cfg.contains(QStringLiteral("y"))) {
        y = cfg[QStringLiteral("y")].toInt();
    }
    weston_output_move(output, x, y);

    Output *o = new Output(output);
    connect(o, &QObject::destroyed, this, &Compositor::outputDestroyed);
    m_outputs.push_back(o);

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
    wl_display_terminate(m_display);
}

Shell *Compositor::shell() const
{
    return m_shell;
}

Orbital::Layer *Compositor::layer(Layer l)
{
    return &m_layers[(int)Layer::Minimized - (int)l];
}

const std::vector<Output *> &Compositor::outputs() const
{
    return m_outputs;
}

std::vector<Seat *> Compositor::seats() const
{
    std::vector<Seat *> seats;
    weston_seat *seat;
    wl_list_for_each(seat, &m_compositor->seat_list, link) {
        seats.push_back(Seat::fromSeat(seat));
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

ChildProcess *Compositor::launchProcess(StringView path)
{
    fmt::print("Launching '{}'...\n", path);
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
    m_hotSpotBindings.emplace(std::make_pair((int)hs, b));
    return b;
}

void Compositor::handleHotSpot(Seat *seat, uint32_t time, PointerHotSpot hs)
{
    auto range = m_hotSpotBindings.equal_range((int)hs);
    std::for_each(range.first, range.second, [seat, time, hs](const std::pair<int, HotSpotBinding *> &pair) {
        pair.second->triggered(seat, time, hs);
    });
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
    auto i = std::find(m_outputs.begin(), m_outputs.end(), o);
    if (i != m_outputs.end()) {
        m_outputs.erase(i);
    }
    emit outputRemoved(o);
}




// -- ChildProcess

struct ChildProcess::Listener
{
    wl_listener listener;
    ChildProcess *parent;
};

ChildProcess::ChildProcess(wl_display *dpy, StringView program)
            : QObject()
            , m_display(dpy)
            , m_program(program.toStdString())
            , m_client(nullptr)
            , m_autoRestart(false)
            , m_listener(new Listener)
            , m_startTime(0)
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
            // 'socket' has SOCK_CLOEXEC, so it will be closed on exec. dup it to carry it on
            int fd = dup(socket);
            setenv("WAYLAND_SOCKET", qPrintable(QString::number(fd)), 1);
            setpriority(PRIO_PROCESS, getpid(), 0);
        }

        int socket;
    };

    Process *process = new Process(sv[1]);
    connect(process, (void (QProcess::*)(QProcess::ProcessError))&QProcess::error, [this](QProcess::ProcessError err) {
        fmt::print("{}: error {}", m_program, (int)err);
    });
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->start(QString::fromStdString(m_program));
    close(sv[1]);
    connect(process, (void (QProcess::*)(int))&QProcess::finished, process, &QObject::deleteLater);

    m_client = wl_client_create(m_display, sv[0]);
    if (!m_client) {
        close(sv[0]);
        fmt::print("wl_client_create failed while launching '{}'.\n", m_program);
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
            fmt::print("{} exited too often too fast. Giving up.\n", m_program);
            m_deathCount = 0;
            emit givingUp();
        } else {
            fmt::print("{} exited. Restarting it...\n", m_program);
            start();
        }
    } else {
        fmt::print("{} exited.\n", m_program);
    }
}

}
