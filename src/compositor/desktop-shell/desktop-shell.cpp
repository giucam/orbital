
#include <QDebug>

#include <wayland-server.h>

#include "desktop-shell.h"
#include "shell.h"
#include "compositor.h"
#include "utils.h"
#include "output.h"
#include "workspace.h"
#include "desktop-shell-workspace.h"
#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {

DesktopShell::DesktopShell(Shell *shell)
       : Interface(shell)
       , Global(shell->compositor(), &desktop_shell_interface, 1)
       , m_shell(shell)
{
    m_client = shell->compositor()->launchProcess(LIBEXEC_PATH "/orbital-client");
}

void DesktopShell::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &desktop_shell_interface, version, id);
    if (client != m_client->client()) {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "permission to bind desktop_shell denied");
        wl_resource_destroy(resource);
    }

    static const struct desktop_shell_interface implementation = {
        wrapInterface(&DesktopShell::setBackground),
        wrapInterface(&DesktopShell::setPanel),
        wrapInterface(&DesktopShell::setLockSurface),
        wrapInterface(&DesktopShell::setPopup),
        wrapInterface(&DesktopShell::unlock),
        wrapInterface(&DesktopShell::setGrabSurface),
        wrapInterface(&DesktopShell::desktopReady),
        wrapInterface(&DesktopShell::addKeyBinding),
        wrapInterface(&DesktopShell::addOverlay),
        wrapInterface(&DesktopShell::minimizeWindows),
        wrapInterface(&DesktopShell::restoreWindows),
        wrapInterface(&DesktopShell::createGrab),
        wrapInterface(&DesktopShell::addWorkspace),
        wrapInterface(&DesktopShell::selectWorkspace),
        wrapInterface(&DesktopShell::quit),
        wrapInterface(&DesktopShell::addTrustedClient),
        wrapInterface(&DesktopShell::pong)
    };

    wl_resource_set_implementation(resource, &implementation, this, nullptr);
    m_resource = resource;

    for (Workspace *ws: m_shell->workspaces()) {
        DesktopShellWorkspace *dws = ws->findInterface<DesktopShellWorkspace>();
        dws->init(m_client->client());
        desktop_shell_send_workspace_added(m_resource, dws->resource());
        dws->sendActivatedState();
    }

    desktop_shell_send_load(resource);
}

void DesktopShell::setBackground(wl_resource *outputResource, wl_resource *surfaceResource)
{
    Output *output = Output::fromResource(outputResource);
    weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surfaceResource));
    output->setBackground(surface);
}

void DesktopShell::setPanel(uint32_t id, wl_resource *outputResource, wl_resource *surfaceResource, uint32_t position)
{
    Output *output = Output::fromResource(outputResource);
    weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surfaceResource));

    if (surface->configure) {
        wl_resource_post_error(surfaceResource, WL_DISPLAY_ERROR_INVALID_OBJECT, "surface role already assigned");
        return;
    }

    class Panel {
    public:
        Panel(wl_resource *res, weston_surface *s, Output *o, int p)
            : m_resource(res)
            , m_surface(s)
            , m_output(o)
//             , m_grab(nullptr)
        {
            struct desktop_shell_panel_interface implementation = {
                wrapInterface(&Panel::move),
                wrapInterface(&Panel::setPosition)
            };

            wl_resource_set_implementation(m_resource, &implementation, this, panelDestroyed);

            setPosition(p);
        }

        void move(wl_client *client, wl_resource *resource)
        {
//             m_grab = new PanelGrab;
//             m_grab->panel = this;
//             weston_seat *seat = container_of(m_shell->compositor()->seat_list.next, weston_seat, link);
//             m_grab->start(seat);
        }
        void setPosition(uint32_t pos)
        {
            m_pos = pos;
            m_output->setPanel(m_surface, m_pos);
        }

        static void panelDestroyed(wl_resource *res)
        {
            Panel *_this = static_cast<Panel *>(wl_resource_get_user_data(res));
//             delete _this->m_grab;
            delete _this;
        }

        wl_resource *m_resource;
        weston_surface *m_surface;
        Output *m_output;
        int m_pos;
//         PanelGrab *m_grab;
    };

    wl_resource *res = wl_resource_create(m_client->client(), &desktop_shell_panel_interface, 1, id);
    new Panel(res, surface, output, position);
}

void DesktopShell::setLockSurface(wl_resource *surface_resource)
{

}

void DesktopShell::setPopup(uint32_t id, wl_resource *parent_resource, wl_resource *surface_resource, int x, int y)
{

}

void DesktopShell::unlock()
{

}

void DesktopShell::setGrabSurface(wl_resource *surface_resource)
{

}

void DesktopShell::desktopReady()
{

}

void DesktopShell::addKeyBinding(uint32_t id, uint32_t key, uint32_t modifiers)
{

}

void DesktopShell::addOverlay(wl_resource *outputResource, wl_resource *surfaceResource)
{
    Output *output = Output::fromResource(outputResource);
    weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surfaceResource));
    output->setOverlay(surface);
}

void DesktopShell::minimizeWindows()
{

}

void DesktopShell::restoreWindows()
{

}

void DesktopShell::createGrab(uint32_t id)
{

}

void DesktopShell::addWorkspace()
{

}

void DesktopShell::selectWorkspace(wl_resource *outputResource, wl_resource *workspaceResource)
{
    Output *output = Output::fromResource(outputResource);
    DesktopShellWorkspace *dws = DesktopShellWorkspace::fromResource(workspaceResource);
    Workspace *ws = dws->workspace();
    qDebug()<<"select"<<output<<ws;
    output->viewWorkspace(ws);
}

void DesktopShell::quit()
{

}

void DesktopShell::addTrustedClient(int32_t fd, const char *interface)
{
}

void DesktopShell::pong(uint32_t serial)
{

}

}
