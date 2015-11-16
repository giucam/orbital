/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <linux/input.h>

#include <wayland-server.h>
#include <QDebug>

#include "../utils.h"
#include "../shell.h"
#include "../surface.h"
#include "../view.h"
#include "../compositor.h"
#include "../layer.h"
#include "../global.h"
#include "../binding.h"
#include "../seat.h"
#include "../output.h"
#include "../layer.h"
#include "../focusscope.h"
#include "../animation.h"
#include "desktop-shell-launcher.h"
#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {

class LauncherSurface : public QObject, public Surface::RoleHandler
{
public:
    LauncherSurface(Shell *shell, Surface *s, wl_resource *res)
        : QObject()
        , m_shell(shell)
        , m_surface(s)
        , m_resource(res)
        , m_active(false)
        , m_fadeIn(false)
        , m_layer(m_shell->compositor()->layer(Compositor::Layer::Overlay))
    {
        static const struct orbital_launcher_surface_interface implementation = {
            wrapInterface(&LauncherSurface::done)
        };
        wl_resource_set_implementation(res, &implementation, this, [](wl_resource *res) {
            delete static_cast<LauncherSurface *>(wl_resource_get_user_data(res));
        });

        s->setLabel(QStringLiteral("launcher"));
        s->setRoleHandler(this);

        m_view = new View(s);
        m_view->setAlpha(0);
        m_view->update();
        m_hidden = true;

        m_layer.addView(m_view);
        m_layer.setMask(0, 0, 0, 0);

        connect(&m_showAnimation, &Animation::update, this, &LauncherSurface::updateAnimation);
        m_showAnimation.setSpeed(0.005);

        connect(s, &Surface::activated, [this]() { m_active = true; });
        connect(s, &Surface::deactivated, [this]() { m_active = false; });
    }

    ~LauncherSurface()
    {
        if (m_resource) {
            wl_resource_set_destructor(m_resource, nullptr);
        }
    }

    void configure(int x, int y) override
    {
        if (m_fadeIn) {
            m_fadeIn = false;
            m_layer.setMask(m_output->x(), m_output->y(), m_output->width(), m_output->height());
            m_shell->appsFocusScope()->activate(m_view->surface());
            m_view->setOutput(m_output);
            m_view->setPos(m_output->x(), m_output->y());
            m_showAnimation.setStart(m_view->alpha());
            m_showAnimation.setTarget(1);
            m_showAnimation.run(m_output);
        }
        m_view->update();
    }

    void toggle(Seat *s, Output *out)
    {
        m_seat = s;
        if (!m_hidden && !m_active) {
            m_shell->appsFocusScope()->activate(m_view->surface());
            return;
        }

        m_hidden = !m_hidden;
        m_output = out;

        if (m_hidden) {
            emit m_view->surface()->unmapped();
            m_showAnimation.setStart(m_view->alpha());
            m_showAnimation.setTarget(0);
            m_showAnimation.run(m_output);
        } else {
            m_fadeIn = true;
            orbital_launcher_surface_send_present(m_resource, out->width(), out->height());
        }
    }

    void updateAnimation(double value)
    {
        m_view->setAlpha(value);
        m_view->update();

        if (value <= 0.01 && m_hidden) {
            m_layer.setMask(0, 0, 0, 0);
        }
    }

    void done()
    {
        if (!m_hidden) {
            toggle(m_seat, m_output);
        }
    }

    void move(Seat *seat) override {}

    Shell *m_shell;
    Surface *m_surface;
    wl_resource *m_resource;
    View *m_view;
    Animation m_showAnimation;
    bool m_hidden;
    bool m_active;
    bool m_fadeIn;
    Seat *m_seat;
    Output *m_output;
    Layer m_layer;
};


DesktopShellLauncher::DesktopShellLauncher(Shell *shell)
                    : Interface(shell)
                    , Global(shell->compositor(), &orbital_launcher_interface, 1)
                    , m_shell(shell)
{
    m_toggleBinding = shell->compositor()->createKeyBinding(KEY_F2, KeyboardModifiers::Super);
    connect(m_toggleBinding, &KeyBinding::triggered, this, &DesktopShellLauncher::toggle);
}

void DesktopShellLauncher::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &orbital_launcher_interface, version, id);

    static const struct orbital_launcher_interface implementation = {
        wrapInterface(&DesktopShellLauncher::getLauncherSurface)
    };

    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void DesktopShellLauncher::getLauncherSurface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surfaceResource)
{
    wl_resource *res = wl_resource_create(client, &orbital_launcher_surface_interface, wl_resource_get_version(resource), id);
    Surface *surf = Surface::fromResource(surfaceResource);

    if (!surf->setRole("orbital_launcher_surface", resource, ORBITAL_LAUNCHER_ERROR_ROLE)) {
        return;
    }

    m_surface = new LauncherSurface(m_shell, surf, res);
}

void DesktopShellLauncher::toggle(Seat *s)
{
    if (!m_surface) {
        return;
    }

    Output *out = m_shell->selectPrimaryOutput(s);
    m_surface->toggle(s, out);
}

}
