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

#include <QDebug>

#include <wayland-server.h>

#include "desktop-shell-workspace.h"
#include "workspace.h"
#include "utils.h"
#include "output.h"
#include "shell.h"
#include "pager.h"
#include "compositor.h"
#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {

DesktopShellWorkspace::DesktopShellWorkspace(Shell *shell, Workspace *ws)
                     : Interface()
                     , m_shell(shell)
                     , m_workspace(ws)
                     , m_resource(nullptr)
{
    connect(shell->pager(), &Pager::workspaceActivated, this, &DesktopShellWorkspace::workspaceActivated);
    connect(shell->compositor(), &Compositor::outputRemoved, this, &DesktopShellWorkspace::outputRemoved);
}

void DesktopShellWorkspace::init(wl_client *client, uint32_t id)
{
    const struct desktop_shell_workspace_interface implementation = {
        wrapInterface(&DesktopShellWorkspace::removed)
    };

    m_resource = wl_resource_create(client, &desktop_shell_workspace_interface, 1, id);
    wl_resource_set_implementation(m_resource, &implementation, this, 0);
}

void DesktopShellWorkspace::sendActivatedState()
{
    for (Output *out: m_active) {
        wl_resource *res = out->resource(wl_resource_get_client(m_resource));
        desktop_shell_workspace_send_activated(m_resource, res);
    }
}

DesktopShellWorkspace *DesktopShellWorkspace::fromResource(wl_resource *res)
{
    return static_cast<DesktopShellWorkspace *>(wl_resource_get_user_data(res));
}

void DesktopShellWorkspace::workspaceActivated(Workspace *w, Output *out)
{
    if (w == m_workspace) {
        m_active.insert(out);

        if (m_resource) {
            wl_resource *res = out->resource(wl_resource_get_client(m_resource));
            desktop_shell_workspace_send_activated(m_resource, res);
        }
    } else if (m_active.contains(out)) {
        m_active.remove(out);

        if (m_resource) {
            wl_resource *res = out->resource(wl_resource_get_client(m_resource));
            desktop_shell_workspace_send_deactivated(m_resource, res);
        }
    }
}

void DesktopShellWorkspace::outputRemoved(Output *o)
{
    m_active.remove(o);
}

void DesktopShellWorkspace::removed(wl_client *client, wl_resource *res)
{

}

}
