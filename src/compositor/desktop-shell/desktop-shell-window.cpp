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

#include "desktop-shell-window.h"
#include "shell.h"
#include "shellsurface.h"
#include "desktop-shell.h"
#include "seat.h"
#include "compositor.h"
#include "shellview.h"
#include "layer.h"

#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {

DesktopShellWindow::DesktopShellWindow(DesktopShell *ds)
                  : Interface()
                  , m_desktopShell(ds)
                  , m_resource(nullptr)
                  , m_state(DESKTOP_SHELL_WINDOW_STATE_INACTIVE)
{
}

DesktopShellWindow::~DesktopShellWindow()
{
    if (m_resource) {
        desktop_shell_window_send_removed(m_resource);
    }
    destroy();
}

void DesktopShellWindow::added()
{
    connect(shsurf(), &ShellSurface::mapped, this, &DesktopShellWindow::mapped);
//     shsurf()->typeChangedSignal.connect(this, &DesktopShellWindow::surfaceTypeChanged);
    connect(shsurf(), &ShellSurface::titleChanged, this, &DesktopShellWindow::sendTitle);
    connect(shsurf(), &Surface::activated, this, &DesktopShellWindow::activated);
    connect(shsurf(), &Surface::deactivated, this, &DesktopShellWindow::deactivated);
//     shsurf()->mappedSignal.connect(this, &DesktopShellWindow::mapped);
//     shsurf()->unmappedSignal.connect(this, &DesktopShellWindow::destroy);
}

ShellSurface *DesktopShellWindow::shsurf()
{
    return static_cast<ShellSurface *>(object());
}

void DesktopShellWindow::mapped()
{
    if (m_resource) {
        return;
    }

    if (shsurf()->type() == ShellSurface::Type::Toplevel) {
        create();
    }
}

void DesktopShellWindow::surfaceTypeChanged()
{
//     ShellSurface::Type type = shsurf()->type();
//     if (type == ShellSurface::Type::TopLevel && !shsurf()->isTransient()) {
//         if (!m_resource) {
//             create();
//         }
//     } else {
//         destroy();
//     }
}

void DesktopShellWindow::activated(Seat *)
{
    m_state |= DESKTOP_SHELL_WINDOW_STATE_ACTIVE;
    sendState();
}

void DesktopShellWindow::deactivated(Seat *)
{
    m_state &= ~DESKTOP_SHELL_WINDOW_STATE_ACTIVE;
    sendState();
}

void DesktopShellWindow::recreate()
{
    if (m_resource) {
        create();
    }
}

void DesktopShellWindow::create()
{
    static const struct desktop_shell_window_interface implementation = {
        wrapInterface(&DesktopShellWindow::setState),
        wrapInterface(&DesktopShellWindow::close)
    };

    m_resource = wl_resource_create(m_desktopShell->client(), &desktop_shell_window_interface, 1, 0);
    wl_resource_set_implementation(m_resource, &implementation, this, 0);
    desktop_shell_send_window_added(m_desktopShell->resource(), m_resource, qPrintable(shsurf()->title()), m_state);
}

void DesktopShellWindow::destroy()
{
    if (m_resource) {
        desktop_shell_window_send_removed(m_resource);
        m_resource = nullptr;
    }
}

void DesktopShellWindow::sendState()
{
    if (m_resource) {
        desktop_shell_window_send_state_changed(m_resource, m_state);
    }
}

void DesktopShellWindow::sendTitle()
{
    if (m_resource) {
        desktop_shell_window_send_set_title(m_resource, qPrintable(shsurf()->title()));
    }
}

void DesktopShellWindow::setState(wl_client *client, wl_resource *resource, int32_t state)
{
    ShellSurface *s = shsurf();
    Seat *seat = m_desktopShell->compositor()->seats().first();

//     if (m_state & DESKTOP_SHELL_WINDOW_STATE_MINIMIZED && !(state & DESKTOP_SHELL_WINDOW_STATE_MINIMIZED)) {
//         s->setMinimized(false);
//     } else if (state & DESKTOP_SHELL_WINDOW_STATE_MINIMIZED && !(m_state & DESKTOP_SHELL_WINDOW_STATE_MINIMIZED)) {
//         s->setMinimized(true);
//         if (s->isActive()) {
//             s->deactivate();
//         }
//     }

    if (state & DESKTOP_SHELL_WINDOW_STATE_ACTIVE && !(state & DESKTOP_SHELL_WINDOW_STATE_MINIMIZED)) {
        seat->activate(s);
        for (Output *o: m_desktopShell->compositor()->outputs()) {
            ShellView *view = s->viewForOutput(o);
            view->layer()->raiseOnTop(view);
        }
    }

//     m_state = state;
//     sendState();
}

void DesktopShellWindow::close(wl_client *client, wl_resource *resource)
{
//     shsurf()->close();
}

}
