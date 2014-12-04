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

#ifndef ORBITAL_DESKTOP_SHELL_WORKSPACE_H
#define ORBITAL_DESKTOP_SHELL_WORKSPACE_H

#include <QSet>

#include "../interface.h"

struct wl_client;
struct wl_resource;

namespace Orbital {

class Workspace;
class Output;
class Shell;

class DesktopShellWorkspace : public Interface
{
    Q_OBJECT
public:
    DesktopShellWorkspace(Shell *shell, Workspace *ws);

    void init(wl_client *client, uint32_t id);
    void sendActivatedState();
    wl_resource *resource() const { return m_resource; }
    Workspace *workspace() const { return m_workspace; }

    static DesktopShellWorkspace *fromResource(wl_resource *resource);

private:
    void workspaceActivated(Workspace *ws, Output *o);
    void outputRemoved(Output *o);
    void removed(wl_client *client, wl_resource *res);

    Shell *m_shell;
    Workspace *m_workspace;
    wl_resource *m_resource;
    QSet<Output *> m_active;
};

}

#endif
