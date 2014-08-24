
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

// void WlShell::sendConfigure(weston_surface *surface, int32_t width, int32_t height)
// {
//     WlShellSurface *wlss = static_cast<WlShellSurface *>(surface->configure_private);
//     wl_shell_surface_send_configure(wlss->resource(), (uint32_t)wlss->shsurf()->resizeEdges(), width, height);
// }
//
ShellSurface *WlShell::getShellSurface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surface_resource)
{
    weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));

    if (surface->configure) {
        wl_resource_post_error(surface_resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "The surface has a role already");
        return nullptr;
    }

    ShellSurface *shsurf = new ShellSurface(m_shell, surface);

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
