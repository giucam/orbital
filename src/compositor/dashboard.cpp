/*
 * Copyright 2015  Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dashboard.h"
#include "shell.h"
#include "compositor.h"
#include "output.h"
#include "global.h"
#include "seat.h"
#include "binding.h"
#include "shellsurface.h"
#include "view.h"
#include "layer.h"

namespace Orbital {

class Dashboard::View : public AbstractWorkspace::View
{
public:
    View(Compositor *c, Output *o)
        : AbstractWorkspace::View(c, o)
        , m_layer(new Layer(c->layer(Compositor::Layer::Dashboard)))
    {
        m_layer->setAcceptInput(false);
    }

    void configure(Orbital::View *view)
    {
        if (view->layer() != m_layer) {
            m_layer->addView(view);
            takeView(view);
        }
    }
    void configureFullscreen(Orbital::View *view, Orbital::View *blackSurface)
    {
        configure(view);
    }

    Layer *m_layer;
};

Dashboard::Dashboard(Shell *shell)
         : QObject(shell)
         , m_shell(shell)
         , m_visible(true)
{
    Compositor *c = shell->compositor();
    m_toggleSurfaceBinding = c->createButtonBinding(PointerButton::Middle, KeyboardModifiers::Super);
    m_toggleBinding = c->createHotSpotBinding(PointerHotSpot::BottomLeftCorner);

    connect(m_toggleSurfaceBinding, &ButtonBinding::triggered, this, &Dashboard::toggleSurface);
    connect(m_toggleBinding, &HotSpotBinding::triggered, this, &Dashboard::toggle);
}

AbstractWorkspace::View *Dashboard::viewForOutput(Output *o)
{
    if (m_views.find(o->id()) == m_views.end()) {
        View *view = new View(m_shell->compositor(), o);
        m_views[o->id()] = view;
        view->setTransformParent(o->rootView());
        return view;
    }

    return m_views[o->id()];
}

void Dashboard::activate(Output *o)
{
}

void Dashboard::toggleSurface(Seat *seat)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    Orbital::View *focus = m_shell->compositor()->pickView(seat->pointer()->x(), seat->pointer()->y());
    if (!focus) {
        return;
    }

    ShellSurface *shsurf = ShellSurface::fromSurface(focus->surface());
    if (!shsurf || shsurf->type() == ShellSurface::Type::Popup) {
        return;
    }

    if (shsurf->workspace() == this) {
        Output *output = m_shell->selectPrimaryOutput();
        shsurf->setWorkspace(output->currentWorkspace());
    } else {
        shsurf->setWorkspace(this);
    }

    // Install a grab to eat the button events
    class Grab : public PointerGrab
    {
    public:
        void motion(uint32_t time, double x, double y) override
        {
            pointer()->move(x, y);
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            if (pointer()->buttonCount() == 0) {
                end();
            }
        }
        void ended() override
        {
            delete this;
        }
    };

    Grab *grab = new Grab;
    grab->start(seat);
}

void Dashboard::toggle(Seat *seat)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    m_visible = !m_visible;

    Output *output = m_shell->selectPrimaryOutput();
    View *wsv = workspaceViewForOutput(this, output);
    Transform tr;
    tr.translate(m_visible ? 0 : -output->width(), m_visible ? 0 : output->height());
    wsv->setTransform(tr, true);
}

}
