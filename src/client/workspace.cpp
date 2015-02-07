/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include "workspace.h"
#include "wayland-desktop-shell-client-protocol.h"
#include "utils.h"
#include "client.h"
#include "uiscreen.h"

Workspace::Workspace(desktop_shell_workspace *ws, QObject *p)
         : QObject(p)
         , m_workspace(ws)
{
    desktop_shell_workspace_add_listener(ws, &m_workspace_listener, this);
}

Workspace::~Workspace()
{
    desktop_shell_workspace_remove(m_workspace);
    desktop_shell_workspace_destroy(m_workspace);
}

bool Workspace::isActiveForScreen(UiScreen *screen) const
{
    wl_output *out = Client::client()->nativeOutput(screen->screen());
    return m_active.contains(out);
}

QPoint Workspace::position() const
{
    return m_position;
}

void Workspace::handleActivated(desktop_shell_workspace *ws, wl_output *out)
{
    m_active.insert(out);
    emit activeChanged();
}

void Workspace::handleDeactivated(desktop_shell_workspace *ws, wl_output *out)
{
    m_active.remove(out);
    emit activeChanged();
}

void Workspace::handlePosition(desktop_shell_workspace *ws, int x, int y)
{
    m_position = QPoint(x, y);
    emit positionChanged();
}

const desktop_shell_workspace_listener Workspace::m_workspace_listener = {
    wrapInterface(&Workspace::handleActivated),
    wrapInterface(&Workspace::handleDeactivated),
    wrapInterface(&Workspace::handlePosition)
};
