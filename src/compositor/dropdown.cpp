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

#include "utils.h"
#include "shell.h"
#include "binding.h"
#include "animation.h"
#include "wayland-dropdown-server-protocol.h"

namespace Orbital {

Dropdown::Dropdown(Shell *shell)
        : Interface(shell)
        , Global(shell->compositor(), &orbital_dropdown_interface, 1)
        , m_shell(shell)
        , m_layer(new Layer(shell->compositor()->stickyLayer()))
{
}

void Dropdown::bind(wl_client *client, uint32_t version, uint32_t id)
{
    static const struct orbital_dropdown_interface implementation = {
        wrapInterface(&Dropdown::getDropdownSurface)
    };

    wl_resource *resource = wl_resource_create(client, &orbital_dropdown_interface, version, id);
    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void Dropdown::getDropdownSurface(wl_client *client, wl_resource *dropdown, uint32_t id, wl_resource *surfaceResource)
{
    wl_resource *resource = wl_resource_create(client, &orbital_dropdown_surface_interface, 1, id);
    weston_surface *ws = static_cast<weston_surface *>(wl_resource_get_user_data(surfaceResource));

    if (ws->configure) {
        wl_resource_post_error(surfaceResource, WL_DISPLAY_ERROR_INVALID_OBJECT, "The surface has a role already");
        wl_resource_destroy(resource);
        return;
    }

    class DropdownSurface : public Surface
    {
    public:
        DropdownSurface(Dropdown *dd, weston_surface *ws, wl_resource *res)
            : Surface(ws)
            , dropdown(dd)
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

            static Role role;

            view = new View(this);
            setRole(&role, [this](int, int) { configure(); });

            m_output = dd->m_shell->selectPrimaryOutput();
            connect(m_output, &Output::availableGeometryChanged, this, &DropdownSurface::updateGeometry);
            updateGeometry();

            Compositor *c = dd->m_shell->compositor();
            m_toggleBinding = c->createKeyBinding(KEY_F12, KeyboardModifiers::None);
            connect(m_toggleBinding, &KeyBinding::triggered, this, &DropdownSurface::toggle);
            connect(&m_animation, &Animation::update, this, &DropdownSurface::updateAnim);
            connect(c, &Compositor::outputRemoved, this, &DropdownSurface::outputRemoved);
        }
        ~DropdownSurface()
        {
            wl_resource_set_destructor(resource, nullptr);
            delete m_toggleBinding;
        }
        void configure()
        {
            if (!isMapped()) {
                dropdown->m_layer->addView(view);
                view->setTransformParent(m_output->rootView());
            }
            view->update();
            if (m_forceReposition || m_lastSize != size()) {
                if (m_visible) {
                    animToPlace();
                } else {
                    view->setPos(posWhen(false));
                    view->update();
                }
                m_lastSize = size();
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
                s->activate(this);
            } else {
                emit unmapped();
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
            connect(m_output, &Output::availableGeometryChanged, this, &DropdownSurface::updateGeometry);
            view->setTransformParent(m_output->rootView());
        }
        QPointF posWhen(bool visible)
        {
            QRect geom = m_output->availableGeometry();
            int x = geom.x() + (geom.width() - width()) / 2.f;
            int y = geom.y() - (!visible) * (height() + geom.y() - m_output->y());
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
        void move(Seat *seat) override
        {
            class MoveGrab : public PointerGrab
            {
            public:
                void motion(uint32_t time, double x, double y) override
                {
                    pointer()->move(x, y);

                    int moveX = x + dx;
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

        Dropdown *dropdown;
        View *view;
        wl_resource *resource;
        KeyBinding *m_toggleBinding;
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

    new DropdownSurface(this, ws, resource);
}

}
