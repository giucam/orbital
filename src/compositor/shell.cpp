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

#include "shell.h"
#include "compositor.h"
#include "layer.h"
#include "workspace.h"
#include "shellsurface.h"
#include "seat.h"
#include "binding.h"
#include "view.h"
#include "shellview.h"
#include "output.h"
#include "xwayland.h"
#include "global.h"

#include "wlshell/wlshell.h"
#include "desktop-shell/desktop-shell.h"
#include "desktop-shell/desktop-shell-workspace.h"

namespace Orbital {

Shell::Shell(Compositor *c)
     : Object()
     , m_compositor(c)
     , m_grabCursorSetter(nullptr)
{
    addInterface(new XWayland(this));
    addInterface(new WlShell(this, m_compositor));
    addInterface(new DesktopShell(this));

    ButtonBinding *binding = c->createButtonBinding(PointerButton::Left, KeyboardModifiers::None);
    connect(binding, &ButtonBinding::triggered, this, &Shell::giveFocus);
    m_focusBinding = binding;
}

Compositor *Shell::compositor() const
{
    return m_compositor;
}

void Shell::addWorkspace(Workspace *ws)
{
    ws->addInterface(new DesktopShellWorkspace(ws));
    m_workspaces << ws;
}

QList<Workspace *> Shell::workspaces() const
{
    return m_workspaces;
}

void Shell::setGrabCursor(Pointer *pointer, PointerCursor c)
{
    if (m_grabCursorSetter) {
        m_grabCursorSetter(pointer, c);
    }
}

void Shell::configure(ShellSurface *shsurf)
{
    if (!shsurf->isMapped()) {
        struct Out {
            Output *output;
            int vote;
        };
        QList<Out> candidates;

        for (Output *o: compositor()->outputs()) {
            candidates.append({ o, 0 });
        }

        Output *output = nullptr;
        if (candidates.isEmpty()) {
            return;
        } else if (candidates.size() == 1) {
            output = candidates.first().output;
        } else {
            QList<Seat *> seats = compositor()->seats();
            for (Out &o: candidates) {
                for (Seat *s: seats) {
                    if (o.output->geometry().contains(s->pointer()->x(), s->pointer()->y())) {
                        o.vote++;
                    }
                }
            }
            Out *out = nullptr;
            for (Out &o: candidates) {
                if (!out || out->vote < o.vote) {
                    out = &o;
                }
            }
            output = out->output;
        }

        shsurf->setWorkspace(output->currentWorkspace());
    }
}

void Shell::setGrabCursorSetter(GrabCursorSetter s)
{
    m_grabCursorSetter = s;
}

void Shell::giveFocus(Seat *seat)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    View *focus = seat->pointer()->focus();
    if (!focus) {
        return;
    }

    seat->activate(focus->surface());

    ShellSurface *shsurf = ShellSurface::fromSurface(focus->surface());
    if (shsurf && shsurf->isFullscreen()) {
        return;
    }

    if (shsurf) {
        for (Output *o: compositor()->outputs()) {
            ShellView *view = shsurf->viewForOutput(o);
            view->layer()->restackView(view);
        }
    }
}

}
