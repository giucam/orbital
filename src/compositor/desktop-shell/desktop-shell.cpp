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

#include "desktop-shell.h"
#include "shell.h"
#include "compositor.h"
#include "utils.h"
#include "output.h"
#include "workspace.h"
#include "view.h"
#include "seat.h"
#include "binding.h"
#include "global.h"
#include "desktop-shell-workspace.h"
#include "desktop-shell-splash.h"
#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {

DesktopShell::DesktopShell(Shell *shell)
            : Interface(shell)
            , Global(shell->compositor(), &desktop_shell_interface, 1)
            , m_shell(shell)
            , m_grabView(nullptr)
            , m_splash(new DesktopShellSplash(shell))
{
    m_shell->addInterface(m_splash);

    m_client = shell->compositor()->launchProcess(LIBEXEC_PATH "/orbital-client");

    shell->setGrabCursorSetter([this](Pointer *p, PointerCursor c) { setGrabCursor(p, c); });
}

DesktopShell::~DesktopShell()
{
    m_shell->setGrabCursorSetter(nullptr);
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

void DesktopShell::setGrabCursor(Pointer *p, PointerCursor c)
{
    p->setFocus(m_grabView, 0, 0);
    desktop_shell_send_grab_cursor(m_resource, (uint32_t)c);
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

void DesktopShell::setGrabSurface(wl_resource *surfaceResource)
{
    weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surfaceResource));
    if (m_grabView) {
        if (surface == m_grabView->surface()) {
            return;
        }
        delete m_grabView;
    }

    m_grabView = new View(weston_view_create(surface));
}

void DesktopShell::desktopReady()
{
    m_splash->hide();
}

void DesktopShell::addKeyBinding(uint32_t id, uint32_t key, uint32_t modifiers)
{
    class Binding : public QObject
    {
    public:
        ~Binding() {
            resource = nullptr;
            delete binding;
        }

        void triggered(Seat *seat, uint32_t time, uint32_t key)
        {
            desktop_shell_binding_send_triggered(resource);
        }

        wl_resource *resource;
        KeyBinding *binding;
    };

    Binding *b = new Binding;
    b->resource = wl_resource_create(m_client->client(), &desktop_shell_binding_interface, wl_resource_get_version(m_resource), id);
    wl_resource_set_implementation(b->resource, nullptr, b, [](wl_resource *r) {
        delete static_cast<Binding *>(wl_resource_get_user_data(r));
    });

    b->binding = m_shell->compositor()->createKeyBinding(key, (KeyboardModifiers)modifiers);
    connect(b->binding, &KeyBinding::triggered, b, &Binding::triggered);
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
    class ClientGrab : public PointerGrab
    {
    public:
        void focus() override
        {
            double sx, sy;
            View *view = pointer()->pickView(&sx, &sy);
            if (currentFocus != view) {
                currentFocus = view;
                desktop_shell_grab_send_focus(resource, view->surface()->resource, wl_fixed_from_double(sx), wl_fixed_from_double(sy));
            }
        }
        void motion(uint32_t time, double x, double y) override
        {
            pointer()->move(x, y);

            QPointF p(pointer()->x(), pointer()->y());
            if (currentFocus) {
                p = currentFocus->mapFromGlobal(p);
            }
            desktop_shell_grab_send_motion(resource, time, wl_fixed_from_double(p.x()), wl_fixed_from_double(p.y()));
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            // Send the event to the application as normal if the mouse was pressed initially.
            // The application has to know the button was released, otherwise its internal state
            // will be inconsistent with the physical button state.
            // Eat the other events, as the app doesn't need to know them.
            // NOTE: this works only if there is only 1 button pressed initially. i can know how many button
            // are pressed but weston currently has no API to determine which ones they are.
            if (pressed && button == pointer()->grabButton()) {
                pointer()->sendButton(time, button, state);
                pressed = false;
            }
            desktop_shell_grab_send_button(resource, time, pointerButtonToRaw(button), (int)state);
        }
        void ended() override
        {
            if (resource) {
                desktop_shell_grab_send_ended(resource);
            }
        }

        void terminate()
        {
            resource = nullptr;
            end();
        }

        wl_resource *resource;
        View *currentFocus;
        bool pressed;
    };

    static const struct desktop_shell_grab_interface desktop_shell_grab_implementation = {
        wrapInterface(&ClientGrab::terminate)
    };


    ClientGrab *grab = new ClientGrab;
    wl_resource *res = wl_resource_create(m_client->client(), &desktop_shell_grab_interface, wl_resource_get_version(m_resource), id);
    wl_resource_set_implementation(res, &desktop_shell_grab_implementation, grab, [](wl_resource *res) {
        delete static_cast<ClientGrab *>(wl_resource_get_user_data(res));
    });

    Seat *seat = m_shell->compositor()->seats().first();
    grab->resource = res;
    grab->pressed = seat->pointer()->buttonCount() > 0;

    double sx, sy;
    View *view = seat->pointer()->pickView(&sx, &sy);
    grab->currentFocus = view;
    grab->start(seat);

    seat->pointer()->setFocus(view, sx, sy);
    desktop_shell_grab_send_focus(grab->resource, view->surface()->resource, sx, sy);
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
    m_shell->compositor()->quit();
}

void DesktopShell::addTrustedClient(int32_t fd, const char *interface)
{
}

void DesktopShell::pong(uint32_t serial)
{

}

}
