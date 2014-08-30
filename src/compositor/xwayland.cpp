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

#include <weston/xwayland.h>

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
    env.insert("WAYLAND_SOCKET", QString::number(dup(sv[1])));

    QString display = QString(":%1").arg(wxs->display);
    QString abstract_fd = QString::number(dup(wxs->abstract_fd));
    QString unix_fd = QString::number(dup(wxs->unix_fd));
    QString wm_fd = QString::number(dup(wm[1]));

    Process *process = new Process;
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->setProcessEnvironment(env);

    process->connect(process, (void (QProcess::*)(int))&QProcess::finished, [wxs, process](int exitCode) {
        weston_xserver_exited(wxs, exitCode);
        delete process;
    });
    process->start(wxs->xserver_path, QStringList() <<
              display <<
              "-rootless" <<
              "-listen" << abstract_fd <<
              "-listen" << unix_fd <<
              "-wm" << wm_fd <<
              "-terminate");

    close(sv[1]);
    wxs->client = wl_client_create(wxs->wl_display, sv[0]);

    close(wm[1]);
    wxs->wm_fd = wm[0];

    return process->processId();
}

XWayland::XWayland(Shell *shell)
        : Interface(shell)
        , m_shell(shell)
{
    const char *path = "/usr/bin/Xwayland";
    weston_compositor *compositor = shell->compositor()->m_compositor;
    m_xwayland = weston_xserver_create(compositor, path);
    m_xwayland->spawn_xserver = spawn_xserver;

    compositor->shell_interface.shell = this;
    compositor->shell_interface.create_shell_surface = [](void *shell, weston_surface *surface, const weston_shell_client *client) {
        XWayland *xwl = static_cast<XWayland *>(shell);
        ShellSurface *shsurf = xwl->m_shell->createShellSurface(surface);
        shsurf->setConfigureSender([client](weston_surface *s, int w, int h) {
            client->send_configure(s, w, h);
        });
        return (shell_surface *)shsurf;
    };

#define _this reinterpret_cast<ShellSurface *>(shsurf)
    compositor->shell_interface.get_primary_view = [](void *shell, shell_surface *shsurf) {
        return (weston_view*)(*_this->m_views.begin())->m_view;
    };
    compositor->shell_interface.set_toplevel = [](shell_surface *shsurf) { _this->setToplevel(); };
    compositor->shell_interface.set_transient = [](shell_surface *shsurf, weston_surface *parent, int x, int y, uint32_t flags) {
        _this->setTransient(parent, x, y, flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE);
    };
//     compositor->shell_interface.set_fullscreen = [](shell_surface *shsurf, uint32_t method, uint32_t framerate, weston_output *output) { _this->setFullscreen((ShellSurface::FullscreenMethod)method, framerate, output);};
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
//     compositor->shell_interface.set_xwayland = [](shell_surface *shsurf, int x, int y, uint32_t flags) { _this->setXWayland(x, y, flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE); };
    compositor->shell_interface.set_title = [](shell_surface *shsurf, const char *t) { _this->setTitle(t); };
    compositor->shell_interface.set_window_geometry = [](shell_surface *shsurf, int32_t x, int32_t y, int32_t w, int32_t h) {
        _this->setGeometry(x, y, w, h);
    };
#undef _this
}

}
