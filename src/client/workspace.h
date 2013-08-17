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

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QObject>

struct desktop_shell_workspace;
struct desktop_shell_workspace_listener;

class Workspace : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
public:
    Workspace(desktop_shell_workspace *ws, QObject *p = nullptr);
    ~Workspace();

    bool active() const { return m_active; }

signals:
    void activeChanged();

private:
    void handleActivated(desktop_shell_workspace *ws);
    void handleDeactivated(desktop_shell_workspace *ws);

    desktop_shell_workspace *m_workspace;
    bool m_active;

    static const desktop_shell_workspace_listener m_workspace_listener;

    friend class Client;
};

#endif
