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
            , m_animValue(1.)
        {
            wl_resource_set_implementation(resource, nullptr, this, [](wl_resource *res) {
                DropdownSurface *ds = static_cast<DropdownSurface *>(wl_resource_get_user_data(res));
                delete ds->view;
            });

            static Role role;

            view = new View(this);
            setRole(&role, [this](int, int) { configure(); });

            Compositor *c = dd->m_shell->compositor();
            m_output = c->outputs().first();

            QRect geom = m_output->availableGeometry();
            orbital_dropdown_surface_send_available_size(res, geom.width(), geom.height());

            m_toggleBinding = c->createKeyBinding(KEY_F11, KeyboardModifiers::None);
            connect(m_toggleBinding, &KeyBinding::triggered, this, &DropdownSurface::toggle);

            m_animation.setSpeed(0.004);
            connect(&m_animation, &Animation::update, this, &DropdownSurface::updateAnim);
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
            }
            view->update();
            if (m_lastSize != size()) {
                setPos();
                m_lastSize = size();
            }
        }
        void toggle(Seat *s)
        {
            m_visible = !m_visible;
            if (m_visible) {
                s->activate(this);
            }

            m_animation.setStart(m_animValue);
            m_animation.setTarget(!m_visible);
            m_animation.run(m_output);
        }
        void updateAnim(double value)
        {
            m_animValue = value;
            setPos();
        }
        void setPos()
        {
            QRect geom = m_output->availableGeometry();
            double x = geom.x() + (geom.width() - width()) / 2.f;
            double y = geom.y() - m_animValue * (height() + geom.y() - m_output->y());
            view->setPos(x, y);
            view->update();
        }

        Dropdown *dropdown;
        View *view;
        wl_resource *resource;
        KeyBinding *m_toggleBinding;
        Animation m_animation;
        bool m_visible;
        Output *m_output;
        double m_animValue;
        QSize m_lastSize;
    };

    new DropdownSurface(this, ws, resource);
}

}
