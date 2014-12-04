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

#include <wayland-server.h>
#include <wayland-server-protocol.h>

#include "wlshellsurface.h"
#include "../utils.h"
#include "../shellsurface.h"
#include "../seat.h"

namespace Orbital {


WlShellSurface::WlShellSurface(WlShell *shell, ShellSurface *shsurf, wl_client *client, uint32_t id)
              : Interface()
              , m_wlShell(shell)
{
    static const struct wl_shell_surface_interface implementation = {
        wrapInterface(&WlShellSurface::pong),
        wrapInterface(&WlShellSurface::move),
        wrapInterface(&WlShellSurface::resize),
        wrapInterface(&WlShellSurface::setToplevel),
        wrapInterface(&WlShellSurface::setTransient),
        wrapInterface(&WlShellSurface::setFullscreen),
        wrapInterface(&WlShellSurface::setPopup),
        wrapInterface(&WlShellSurface::setMaximized),
        wrapInterface(&WlShellSurface::setTitle),
        wrapInterface(&WlShellSurface::setClass)
    };

    m_resource = wl_resource_create(client, &wl_shell_surface_interface, 1, id);
    wl_resource_set_implementation(m_resource, &implementation, this,
                                   [](wl_resource *resource) {
                                       static_cast<WlShellSurface *>(wl_resource_get_user_data(resource))->resourceDestroyed();
                                   });

    shsurf->setConfigureSender([this](int w, int h) {
        wl_shell_surface_send_configure(m_resource, 0, w, h);
    });
    connect(shsurf, &ShellSurface::popupDone, this, &WlShellSurface::popupDone);
}

WlShellSurface::~WlShellSurface()
{
    if (m_resource) {
        wl_resource_set_destructor(m_resource, nullptr);
    }
}

void WlShellSurface::resourceDestroyed()
{
    m_resource = nullptr;
    shellSurface()->setConfigureSender(nullptr);
    shellSurface()->unmap();
    delete this;
}

ShellSurface *WlShellSurface::shellSurface() const
{
    return static_cast<ShellSurface *>(object());
}

void WlShellSurface::pong(uint32_t serial)
{

}

void WlShellSurface::move(wl_resource *seatResource, uint32_t serial)
{
    Seat *seat = Seat::fromResource(seatResource);

    if (seat->pointer()->buttonCount() == 0 || seat->pointer()->grabSerial() != serial) {
        return;
    }

    shellSurface()->move(seat);
}

void WlShellSurface::resize(wl_resource *seatResource, uint32_t serial, uint32_t edges)
{
    Seat *seat = Seat::fromResource(seatResource);

    if (seat->pointer()->buttonCount() == 0 || seat->pointer()->grabSerial() != serial) {
        return;
    }

    shellSurface()->resize(seat, (ShellSurface::Edges)edges);
}

void WlShellSurface::setToplevel()
{
    shellSurface()->setToplevel();
}

void WlShellSurface::setTransient(wl_resource *parentResource, int x, int y, uint32_t flags)
{
    Surface *parent = Surface::fromResource(parentResource);
    shellSurface()->setTransient(parent, x, y, flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE);
}

void WlShellSurface::setFullscreen(uint32_t method, uint32_t framerate, wl_resource *outputResource)
{
    // blatantly ignore the output and method for now
    shellSurface()->setFullscreen();
}

void WlShellSurface::setPopup(wl_resource *seatResource, uint32_t serial, wl_resource *parentResource, int32_t x, int32_t y, uint32_t flags)
{
    Surface *parent = Surface::fromResource(parentResource);
    Seat *seat = Seat::fromResource(seatResource);

    if (serial != seat->pointer()->grabSerial()) {
        wl_shell_surface_send_popup_done(m_resource);
        return;
    }

    shellSurface()->setPopup(parent, seat, x, y);
}

void WlShellSurface::setMaximized(wl_resource *outputResource)
{
    shellSurface()->setMaximized();
}

void WlShellSurface::setTitle(const char *title)
{
    shellSurface()->setTitle(title);
}

void WlShellSurface::setClass(const char *className)
{
    shellSurface()->setAppId(className);
}

void WlShellSurface::popupDone()
{
    wl_shell_surface_send_popup_done(m_resource);
}

};
