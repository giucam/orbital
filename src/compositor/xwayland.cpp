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
#include "surface.h"
#include "format.h"

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

    delete _this->m_process;
    _this->m_process = new Process;
    _this->m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    _this->m_process->setProcessEnvironment(env);

    _this->m_process->connect(_this->m_process, (void (QProcess::*)(int))&QProcess::finished, [_this](int exitCode) {
        _this->m_api->xserver_exited(_this->m_xwayland, exitCode);
        // The process should now be freed but QProcess doesn't like to be delete'd here directly,
        // so we keep it alive and delete it when we quit or when we create a new process
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
    XWlSurface(const weston_xwayland_surface_api *a, ShellSurface *ss)
        : api(a)
        , shsurf(ss)
    {
        connect(ss->surface(), &Surface::pointerFocusEnter, this, &XWlSurface::enter);
        connect(ss->surface(), &Surface::pointerFocusLeave, this, &XWlSurface::leave);
        leave();
    }

    void enter()
    {
        api->send_position(shsurf->surface()->surface(), 0, 0);
    }

    void leave()
    {
        api->send_position(shsurf->surface()->surface(), 10000, 10000);
    }

    const weston_xwayland_surface_api *api;
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
    if (!m_api) {
        fmt::print("Cannot load the xwayland module! Compatibility with X apps is disabled.\n");
        return;
    }

    m_xwayland = m_api->get(compositor);

    m_api->listen(m_xwayland, this, spawnXserver);

    wl_event_loop *loop = wl_display_get_event_loop(compositor->wl_display);
    m_sigusr1Source = wl_event_loop_add_signal(loop, SIGUSR1, [](int, void *ud) {
        XWayland *_this = static_cast<XWayland *>(ud);
        _this->m_api->xserver_loaded(_this->m_xwayland, _this->m_client, _this->m_wmFd);
        wl_event_source_remove(_this->m_sigusr1Source);
        return 1;
    }, this);

    auto surfaceApi = weston_xwayland_surface_get_api(compositor);
    if (surfaceApi) {
        connect(shell, &Shell::shellSurfaceCreated, this, [this, surfaceApi](ShellSurface *shsurf) {
            if (surfaceApi->is_xwayland_surface(shsurf->surface()->surface())) {
                XWlSurface *xs = new XWlSurface(surfaceApi, shsurf);
                shsurf->addInterface(xs);
            }
        });
    }
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
