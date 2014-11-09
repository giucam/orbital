/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_DESKTOP_SHELL_NOTIFICATIONS_H
#define ORBITAL_DESKTOP_SHELL_NOTIFICATIONS_H

#include <wayland-server.h>

#include "interface.h"

namespace Orbital {

class Shell;

class DesktopShellNotifications : public Interface, public Global
{
    Q_OBJECT
public:
    DesktopShellNotifications(Shell *shell);
    ~DesktopShellNotifications();

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    class NotificationSurface;
    void pushNotification(uint32_t id, wl_resource *surfaceResource, int32_t flags);
    void relayout();

    Shell *m_shell;
    QList<NotificationSurface *> m_notifications;
};

}

#endif
