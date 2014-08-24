
#include <wayland-server.h>
#include <wayland-server-protocol.h>

#include "wlshellsurface.h"
#include "utils.h"
#include "shellsurface.h"
#include "seat.h"

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

    connect(shsurf, &ShellSurface::popupDone, this, &WlShellSurface::popupDone);
}

void WlShellSurface::resourceDestroyed()
{
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

void WlShellSurface::resize(wl_resource *seat_resource, uint32_t serial, uint32_t edges)
{

}

void WlShellSurface::setToplevel()
{
    shellSurface()->setToplevel();
}

void WlShellSurface::setTransient(wl_resource *parent_resource, int x, int y, uint32_t flags)
{

}

void WlShellSurface::setFullscreen(uint32_t method, uint32_t framerate, wl_resource *output_resource)
{

}

void WlShellSurface::setPopup(wl_resource *seatResource, uint32_t serial, wl_resource *parentResource, int32_t x, int32_t y, uint32_t flags)
{
    weston_surface *parent = static_cast<weston_surface *>(wl_resource_get_user_data(parentResource));
    Seat *seat = Seat::fromResource(seatResource);

    if (serial != seat->pointer()->grabSerial()) {
        wl_shell_surface_send_popup_done(m_resource);
        return;
    }

    shellSurface()->setPopup(parent, seat, x, y);
}

void WlShellSurface::setMaximized(wl_resource *output_resource)
{

}

void WlShellSurface::setTitle(const char *title)
{

}

void WlShellSurface::setClass(const char *className)
{

}

void WlShellSurface::popupDone()
{
    wl_shell_surface_send_popup_done(m_resource);
}

};
