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

#ifndef ORBITAL_XWAYLAND_H
#define ORBITAL_XWAYLAND_H

#include <xwayland-api.h>

#include "interface.h"

struct wl_event_source;
struct weston_xserver;

namespace Orbital {

class Shell;

class XWayland : public Interface
{
public:
    XWayland(Shell *shell);
    ~XWayland();

private:
    static pid_t spawnXserver(void *ud, const char *xdpy, int abstractFd, int unixFd);
    class Process;

    Shell *m_shell;
    const weston_xwayland_api *m_api;
    weston_xwayland *m_xwayland;
    Process *m_process;
    wl_client *m_client;
    int m_wmFd;
    wl_event_source *m_sigusr1Source;
};

}

#endif
