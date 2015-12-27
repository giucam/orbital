/*
 * Copyright 2015 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <unistd.h>
#include <linux/input.h>

#include <wayland-server.h>
#include <QDebug>

#include "../utils.h"
#include "../shell.h"
#include "../global.h"
#include "../compositor.h"
#include "../seat.h"
#include "desktop-shell-settings.h"
#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {

DesktopShellSettings::DesktopShellSettings(Shell *shell)
                    : Interface(shell)
                    , Global(shell->compositor(), &orbital_settings_interface, 1)
                    , m_shell(shell)
{
}

void DesktopShellSettings::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &orbital_settings_interface, version, id);

// FIXME
//     // We trust only client we started ourself
//     pid_t pid;
//     wl_client_get_credentials(client, &pid, nullptr, nullptr);
//     if (pid != getpid()) {
//         wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "permission to bind orbital_settings denied");
//         wl_resource_destroy(resource);
//         return;
//     }

    static const struct orbital_settings_interface implementation = {
        wrapInterface(&DesktopShellSettings::destroy),
        wrapInterface(&DesktopShellSettings::setKeymap),
    };

    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void DesktopShellSettings::destroy(wl_client *client, wl_resource *resource)
{
    wl_resource_destroy(resource);
}

void DesktopShellSettings::setKeymap(const char *layout)
{
    foreach (Seat *seat, m_shell->compositor()->seats()) {
        seat->setKeymap(Keymap(StringView(layout), Maybe<StringView>()));
    }
}

}
