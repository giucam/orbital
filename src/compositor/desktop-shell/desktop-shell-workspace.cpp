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
#include "../workspace.h"
#include "../utils.h"
#include "../output.h"
#include "../shell.h"
#include "../pager.h"
#include "../compositor.h"
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
    connect(ws, &Workspace::positionChanged, this, &DesktopShellWorkspace::sendPosition);
}

void DesktopShellWorkspace::init(wl_client *client, uint32_t id)
{
    const struct desktop_shell_workspace_interface implementation = {
        wrapInterface(&DesktopShellWorkspace::removed)
    };

    m_resource = wl_resource_create(client, &desktop_shell_workspace_interface, 1, id);
    wl_resource_set_implementation(m_resource, &implementation, this, [](wl_resource *r) {
        DesktopShellWorkspace *ws = fromResource(r);
        ws->m_resource = nullptr;
    });
}

void DesktopShellWorkspace::sendActivatedState()
{
    for (Output *out: m_active) {
        wl_resource *res = out->resource(wl_resource_get_client(m_resource));
        desktop_shell_workspace_send_activated(m_resource, res);
    }
}

void DesktopShellWorkspace::sendPosition()
{
    if (m_resource) {
        desktop_shell_workspace_send_position(m_resource, m_workspace->x(), m_workspace->y());
    }
}

DesktopShellWorkspace *DesktopShellWorkspace::fromResource(wl_resource *res)
{
    return static_cast<DesktopShellWorkspace *>(wl_resource_get_user_data(res));
}

void DesktopShellWorkspace::workspaceActivated(Workspace *w, Output *out)
{
    if (w == m_workspace) {
        m_active.push_back(out);

        if (m_resource) {
            wl_resource *res = out->resource(wl_resource_get_client(m_resource));
            if (res) {
                desktop_shell_workspace_send_activated(m_resource, res);
            }
        }
    } else if (std::find(m_active.begin(), m_active.end(), out) != m_active.end()) {
        m_active.remove(out);

        if (m_resource) {
            wl_resource *res = out->resource(wl_resource_get_client(m_resource));
            if (res) {
                desktop_shell_workspace_send_deactivated(m_resource, res);
            }
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
