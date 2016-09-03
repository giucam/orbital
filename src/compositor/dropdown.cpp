/*
 * Copyright 2013-2014  Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <linux/input.h>

#include <QDebug>

#include "dropdown.h"
#include "surface.h"
#include "layer.h"
#include "compositor.h"
#include "view.h"
#include "output.h"
#include "global.h"
#include "seat.h"
#include "focusscope.h"
#include "utils.h"
#include "shell.h"
#include "binding.h"
#include "animation.h"
#include "wayland-dropdown-server-protocol.h"

namespace Orbital {

class Dropdown::DropdownSurface : public QObject, public Surface::RoleHandler
{
public:
    DropdownSurface(Dropdown *dd, Shell *sh, Surface *s, wl_resource *res)
        : surface(s)
        , dropdown(dd)
        , shell(sh)
        , resource(res)
        , m_visible(false)
        , m_moving(false)
        , m_forceReposition(false)
        , m_firstShow(true)
    {
        wl_resource_set_implementation(resource, nullptr, this, [](wl_resource *res) {
            DropdownSurface *ds = static_cast<DropdownSurface *>(wl_resource_get_user_data(res));
            delete ds->view;
        });

        view = new View(s);
        s->setRoleHandler(this);
        s->setMoveHandler([this](Seat *s) { move(s); });
        s->setLabel("dropdown");

        m_output = dd->m_shell->selectPrimaryOutput();
        connect(m_output, &Output::availableGeometryChanged, this, &DropdownSurface::updateGeometry);
        updateGeometry();

        Compositor *c = dd->m_shell->compositor();
        connect(&m_animation, &Animation::update, this, &DropdownSurface::updateAnim);
        connect(c, &Compositor::outputRemoved, this, &DropdownSurface::outputRemoved);
        connect(s, &QObject::destroyed, [this]() { delete this; });
    }
    ~DropdownSurface()
    {
        dropdown->m_surface = nullptr;
        wl_resource_set_destructor(resource, nullptr);
    }
    void configure(int x, int y) override
    {
        if (!surface->isMapped()) {
            dropdown->m_layer->addView(view);
            view->setTransformParent(m_output->rootView());
        }
        view->update();
        if (m_forceReposition || m_lastSize != surface->size()) {
            if (m_visible) {
                animToPlace();
            } else {
                view->setPos(posWhen(false));
                view->update();
            }
            m_lastSize = surface->size();
            m_forceReposition = false;
        }
    }
    void toggle(Seat *s)
    {
        if (m_moving) {
            return;
        }

        m_visible = !m_visible;
        if (m_visible) {
            shell->appsFocusScope()->activate(surface);
        } else {
            emit surface->unmapped();
        }
        if (m_visible && m_firstShow) {
            setOutput(dropdown->m_shell->selectPrimaryOutput());
            updateGeometry();
            m_firstShow = false;
        } else {
            animToPlace();
        }
    }
    void setOutput(Output *o)
    {
        disconnect(m_output, &Output::availableGeometryChanged, this, &DropdownSurface::updateGeometry);
        m_output = o;
        if (o) {
            connect(m_output, &Output::availableGeometryChanged, this, &DropdownSurface::updateGeometry);
            view->setTransformParent(m_output->rootView());
        }
    }
    QPointF posWhen(bool visible)
    {
        QRect geom = m_output->availableGeometry();
        int x = geom.x() + (geom.width() - surface->width()) / 2.f;
        int y = geom.y() - (!visible) * (surface->height() + geom.y() - m_output->y());
        return QPointF(x, y);
    }
    void updateAnim(double value)
    {
        QPointF pos = m_start * (1. - value) + m_end * value;

        view->setPos(pos);
        view->update();
    }
    void updateGeometry()
    {
        if (!m_output) {
            return;
        }

        QRect geom = m_output->availableGeometry();
        orbital_dropdown_surface_send_available_size(resource, geom.width(), geom.height());
        m_forceReposition = true;
    }
    void snapToPlace(Output *o)
    {
        view->setTransformParent(o->rootView());

        view->setPos(view->pos() - o->rootView()->pos());
        m_start = view->pos();

        if (m_output != o) {
            setOutput(o);
            updateGeometry();
        } else {
            animToPlace();
        }
    }
    void animToPlace()
    {
        m_start = view->pos();
        m_end = posWhen(m_visible);

        m_animation.setStart(0);
        m_animation.setTarget(1);
        m_animation.run(m_output, (m_start - m_end).manhattanLength() * 0.5);
    }
    void move(Seat *seat)
    {
        class MoveGrab : public PointerGrab
        {
        public:
            void motion(uint32_t time, Pointer::MotionEvent evt) override
            {
                pointer()->move(evt);

                QPointF pos = pointer()->motionToAbs(evt);
                int moveX = pos.x() + dx;
                surface->view->setPos(moveX, surface->view->y());
            }
            void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
            {
                if (pointer()->buttonCount() == 0 && state == Pointer::ButtonState::Released) {
                    end();
                }
            }
            void ended() override
            {
                surface->m_moving = false;
                surface->snapToPlace(surface->view->output());
                delete this;
            }

            DropdownSurface *surface;
            double dx, dy;
        };

        view->setTransformParent(nullptr);
        view->setPos(view->pos() + m_output->rootView()->pos());
        view->update();

        MoveGrab *move = new MoveGrab;
        View *view = seat->pointer()->pickView();
        move->dx = view->x() - seat->pointer()->x();
        move->dy = view->y() - seat->pointer()->y();
        move->surface = this;
        m_moving = true;

        move->start(seat, PointerCursor::Move);
    }
    void outputRemoved(Output *o)
    {
        if (m_output == o) {
            setOutput(dropdown->m_shell->selectPrimaryOutput());
            updateGeometry();
        }
    }

    Surface *surface;
    Dropdown *dropdown;
    View *view;
    Shell *shell;
    wl_resource *resource;
    Animation m_animation;
    bool m_visible;
    Output *m_output;
    QSize m_lastSize;
    QPointF m_start;
    QPointF m_end;
    bool m_moving;
    bool m_forceReposition;
    bool m_firstShow;
};

Dropdown::Dropdown(Shell *shell)
        : Interface(shell)
        , Global(shell->compositor(), &orbital_dropdown_interface, 1)
        , m_shell(shell)
        , m_layer(new Layer(shell->compositor()->layer(Compositor::Layer::Sticky)))
        , m_surface(nullptr)
{
    shell->addAction("ToggleDropdown", [this](Seat *s) {
        if (m_surface) {
            m_surface->toggle(s);
        }
    });
}

Dropdown::~Dropdown()
{
    delete m_surface;
}

void Dropdown::bind(wl_client *client, uint32_t version, uint32_t id)
{
    static const struct orbital_dropdown_interface implementation = {
        wrapInterface(getDropdownSurface)
    };

    wl_resource *resource = wl_resource_create(client, &orbital_dropdown_interface, version, id);
    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void Dropdown::getDropdownSurface(wl_client *client, wl_resource *dropdown, uint32_t id, wl_resource *surfaceResource)
{
    wl_resource *resource = wl_resource_create(client, &orbital_dropdown_surface_interface, 1, id);
    Surface *s = Surface::fromResource(surfaceResource);

    if (!s->setRole("orbital_dropdown_surface", dropdown, ORBITAL_DROPDOWN_ERROR_ROLE)) {
        return;
    }

    delete m_surface;
    m_surface = new DropdownSurface(this, m_shell, s, resource);
}

}
