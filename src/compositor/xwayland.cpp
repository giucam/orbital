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

#include <weston-1/xwayland.h>

#include "xwayland.h"
#include "shell.h"
#include "compositor.h"
#include "shellsurface.h"
#include "shellview.h"
#include "seat.h"

namespace Orbital {

class Process : public QProcess
{
public:
    void setupChildProcess()
    {
        signal(SIGUSR1, SIG_IGN);
    }
};

static Process *s_process = nullptr;

static pid_t
spawn_xserver(struct weston_xserver *wxs)
{
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

    QString display = QStringLiteral(":%1").arg(wxs->display);
    QString abstract_fd = QString::number(dup(wxs->abstract_fd));
    QString unix_fd = QString::number(dup(wxs->unix_fd));
    QString wm_fd = QString::number(dup(wm[1]));

    s_process = new Process;
    s_process->setProcessChannelMode(QProcess::ForwardedChannels);
    s_process->setProcessEnvironment(env);

    s_process->connect(s_process, (void (QProcess::*)(int))&QProcess::finished, [wxs](int exitCode) {
        weston_xserver_exited(wxs, exitCode);
        if (s_process) {
            delete s_process;
            s_process = nullptr;
        }
    });
    s_process->start(QStringLiteral("Xwayland"), {
              display,
              QStringLiteral("-rootless"),
              QStringLiteral("-listen"), abstract_fd,
              QStringLiteral("-listen"), unix_fd,
              QStringLiteral("-wm"), wm_fd,
              QStringLiteral("-terminate") });

    close(sv[1]);
    wxs->client = wl_client_create(wxs->wl_display, sv[0]);

    close(wm[1]);
    wxs->wm_fd = wm[0];

    return s_process->processId();
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
{
    weston_compositor *compositor = shell->compositor()->m_compositor;
    m_xwayland = weston_xserver_create(compositor);
    m_xwayland->spawn_xserver = spawn_xserver;

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
    compositor->shell_interface.resize = [](shell_surface *shsurf, weston_seat *ws, uint32_t edges) {
        if (ws) {
            _this->resize(Seat::fromSeat(ws), (ShellSurface::Edges)edges);
        }
        return 0;
    };
    compositor->shell_interface.move = [](shell_surface *shsurf, weston_seat *ws) {
        if (ws) {
            _this->move(Seat::fromSeat(ws));
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
#undef _this
}

XWayland::~XWayland()
{
    if (s_process) {
        Process *proc = s_process;
        s_process = nullptr;

        proc->kill();
        proc->waitForFinished();
        delete proc;
    }
}

}
