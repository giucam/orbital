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
#include <wayland-server-protocol.h>

#include "shell.h"
#include "wlshell.h"
#include "utils.h"
#include "wlshellsurface.h"
#include "shellsurface.h"

namespace Orbital {

WlShell::WlShell(Shell *shell, Compositor *c)
       : Interface(shell)
       , Global(c, &wl_shell_interface, 1)
       , m_shell(shell)
{
//     weston_seat *seat;
//     wl_list_for_each(seat, &Shell::compositor()->seat_list, link) {
//         ShellSeat *shseat = ShellSeat::shellSeat(seat);
//         shseat->pointerFocusSignal.connect(this, &WlShell::pointerFocus);
//     }
}

void WlShell::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &wl_shell_interface, version, id);
    if (resource)
        wl_resource_set_implementation(resource, &shell_implementation, this, nullptr);
}

ShellSurface *WlShell::getShellSurface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surface_resource)
{
    weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));

    if (surface->configure) {
        wl_resource_post_error(surface_resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "The surface has a role already");
        return nullptr;
    }

    ShellSurface *shsurf = m_shell->createShellSurface(surface);
    WlShellSurface *wlss = new WlShellSurface(this, shsurf, client, id);
    shsurf->addInterface(wlss);

//     wlss->responsivenessChangedSignal.connect(this, &WlShell::surfaceResponsiveness);
//
    return shsurf;
}
//
// void WlShell::pointerFocus(ShellSeat *, weston_pointer *pointer)
// {
//     weston_view *view = pointer->focus;
//
//     if (!view)
//         return;
//
//     ShellSurface *shsurf = Shell::getShellSurface(view->surface);
//     if (!shsurf)
//         return;
//
//     WlShellSurface *wlss = shsurf->findInterface<WlShellSurface>();
//     if (!wlss) {
//         return;
//     }
//
//     if (!wlss->isResponsive()) {
//         surfaceResponsivenessChangedSignal(shsurf, false);
//     } else {
//         uint32_t serial = wl_display_next_serial(Shell::compositor()->wl_display);
//         wlss->ping(serial);
//     }
// }
//
// void WlShell::surfaceResponsiveness(WlShellSurface *wlsurf)
// {
//     ShellSurface *shsurf = wlsurf->shsurf();
//     surfaceResponsivenessChangedSignal(shsurf, wlsurf->isResponsive());
// }
//
// const weston_shell_client WlShell::shell_client = {
//     WlShell::sendConfigure
// };
//
const struct wl_shell_interface WlShell::shell_implementation = {
    wrapInterface(&WlShell::getShellSurface)
};

}
