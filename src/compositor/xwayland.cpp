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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <QProcess>
#include <QProcessEnvironment>
#include <QDebug>

#include "xwayland.h"
#include "shell.h"
#include "compositor.h"
#include "shellsurface.h"
#include "shellview.h"
#include "seat.h"

namespace Orbital {

class XWayland::Process : public QProcess
{
public:
    void setupChildProcess()
    {
        signal(SIGUSR1, SIG_IGN);
    }
};

pid_t XWayland::spawnXserver(void *ud, const char *xdpy, int abstractFd, int unixFd)
{
    XWayland *_this = static_cast<XWayland *>(ud);

    int sv[2], wm[2];

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) {
        weston_log("wl connection socketpair failed\n");
        return 1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, wm) < 0) {
        weston_log("X wm connection socketpair failed\n");
        return 1;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WAYLAND_SOCKET"), QString::number(dup(sv[1])));

    QString abstract_fd_str = QString::number(dup(abstractFd));
    QString unix_fd_str = QString::number(dup(unixFd));
    QString wm_fd_str = QString::number(dup(wm[1]));

    _this->m_process = new Process;
    _this->m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    _this->m_process->setProcessEnvironment(env);

    _this->m_process->connect(_this->m_process, (void (QProcess::*)(int))&QProcess::finished, [_this](int exitCode) {
        _this->m_api->xserver_exited(_this->m_xwayland, exitCode);
        if (_this->m_process) {
            delete _this->m_process;
            _this->m_process = nullptr;
        }
    });
    _this->m_process->start(QStringLiteral("Xwayland"), {
                            QLatin1String(xdpy),
                            QStringLiteral("-rootless"),
                            QStringLiteral("-listen"), abstract_fd_str,
                            QStringLiteral("-listen"), unix_fd_str,
                            QStringLiteral("-wm"), wm_fd_str,
                            QStringLiteral("-terminate") });

    close(sv[1]);
    _this->m_client = wl_client_create(_this->m_shell->compositor()->display(), sv[0]);

    close(wm[1]);
    _this->m_wmFd = wm[0];

    return _this->m_process->processId();
}

class XWlSurface : public Interface {
public:
    XWlSurface(const weston_shell_client *c, ShellSurface *ss)
        : client(c)
        , shsurf(ss)
    {
        connect(ss->surface(), &Surface::pointerFocusEnter, this, &XWlSurface::enter);
        connect(ss->surface(), &Surface::pointerFocusLeave, this, &XWlSurface::leave);
        leave();
    }

    void enter()
    {
        client->send_position(shsurf->surface()->surface(), 0, 0);
    }

    void leave()
    {
        client->send_position(shsurf->surface()->surface(), 10000, 10000);
    }

    const weston_shell_client *client;
    ShellSurface *shsurf;
};

XWayland::XWayland(Shell *shell)
        : Interface(shell)
        , m_shell(shell)
        , m_process(nullptr)
{
    weston_compositor *compositor = shell->compositor()->m_compositor;

    weston_compositor_load_xwayland(compositor);
    m_api = weston_xwayland_get_api(compositor);
    m_xwayland = m_api->get(compositor);

    m_api->listen(m_xwayland, this, spawnXserver);

    wl_event_loop *loop = wl_display_get_event_loop(compositor->wl_display);
    m_sigusr1Source = wl_event_loop_add_signal(loop, SIGUSR1, [](int, void *ud) {
        XWayland *_this = static_cast<XWayland *>(ud);
        _this->m_api->xserver_loaded(_this->m_xwayland, _this->m_client, _this->m_wmFd);
        wl_event_source_remove(_this->m_sigusr1Source);
        return 1;
    }, this);

    compositor->shell_interface.shell = this;
    compositor->shell_interface.create_shell_surface = [](void *shell, weston_surface *surface, const weston_shell_client *client) {
        XWayland *xwl = static_cast<XWayland *>(shell);
        Surface *surf = Surface::fromSurface(surface);
        surf->setRole("xwayland_surface");
        ShellSurface *shsurf = xwl->m_shell->createShellSurface(surf);
        shsurf->setConfigureSender([client, surface](int w, int h) {
            client->send_configure(surface, w, h);
        });
        XWlSurface *xs = new XWlSurface(client, shsurf);
        shsurf->addInterface(xs);
        return (shell_surface *)xs;
    };

#define _this reinterpret_cast<XWlSurface *>(shsurf)->shsurf
    compositor->shell_interface.set_toplevel = [](shell_surface *shsurf) { _this->setToplevel(); };
    compositor->shell_interface.set_transient = [](shell_surface *shsurf, weston_surface *parent, int x, int y, uint32_t flags) {
        _this->setTransient(Surface::fromSurface(parent), x, y, flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE);
    };
    compositor->shell_interface.set_fullscreen = [](shell_surface *shsurf, uint32_t method, uint32_t framerate, weston_output *output) {
        _this->setFullscreen();
    };
    compositor->shell_interface.resize = [](shell_surface *shsurf, weston_pointer *p, uint32_t edges) {
        if (p) {
            _this->resize(Seat::fromSeat(p->seat), (ShellSurface::Edges)edges);
        }
        return 0;
    };
    compositor->shell_interface.move = [](shell_surface *shsurf, weston_pointer *p) {
        if (p) {
            _this->move(Seat::fromSeat(p->seat));
        }
        return 0;
    };
    compositor->shell_interface.set_xwayland = [](shell_surface *shsurf, int x, int y, uint32_t flags) {
        _this->setXWayland(x, y, flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE);
    };
    compositor->shell_interface.set_title = [](shell_surface *shsurf, const char *t) {
        _this->setTitle(t);
    };
    compositor->shell_interface.set_window_geometry = [](shell_surface *shsurf, int32_t x, int32_t y, int32_t w, int32_t h) {
        _this->setGeometry(x, y, w, h);
    };
    compositor->shell_interface.set_maximized = [](shell_surface *shsurf) { _this->setMaximized(); };
    compositor->shell_interface.set_pid = [](shell_surface *shsurf, pid_t pid) { _this->setPid(pid); };
//     compositor->shell_interface.get_output_work_area = [](void *shell, weston_output *output, pixman_rectangle32_t *area) {}
#undef _this
}

XWayland::~XWayland()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished();
        delete m_process;
    }
}

}
