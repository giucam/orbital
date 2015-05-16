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

#include "../shell.h"
#include "../compositor.h"
#include "../utils.h"
#include "../output.h"
#include "../layer.h"
#include "../view.h"
#include "../animation.h"
#include "../surface.h"
#include "desktop-shell-splash.h"
#include "wayland-desktop-shell-server-protocol.h"

namespace Orbital {


class Splash : public QObject, public Surface::RoleHandler
{
public:
    Splash(DesktopShellSplash *p, View *v)
        : parent(p)
        , view(v)
        , fadeAnimation(new Animation)
    {
        fadeAnimation->connect(fadeAnimation, &Animation::update, [this](double v) { view->setAlpha(v); });
        fadeAnimation->connect(fadeAnimation, &Animation::done, [this]() { done(); });
    }

    ~Splash()
    {
        delete fadeAnimation;
        delete view;
    }

    void fadeOut()
    {
        fadeAnimation->setStart(1.f);
        fadeAnimation->setTarget(0.f);
        fadeAnimation->run(view->output(), 500, Animation::Flags::SendDone);
    }

    void done()
    {
        parent->m_splashes.remove(this);
        if (parent->m_splashes.isEmpty()) {
            desktop_shell_splash_send_done(parent->m_resource);
        }
        delete this;
    }

    void outputDestroyed()
    {
        parent->m_splashes.remove(this);
        delete this;
    }

    void configure(int x, int y) override
    {
        view->output()->repaint();
    }

    void move(Seat *seat) override {}

private:
    DesktopShellSplash *parent;
    View *view;
    Animation *fadeAnimation;
};

DesktopShellSplash::DesktopShellSplash(Shell *shell)
                  : Interface(shell)
                  , Global(shell->compositor(), &desktop_shell_splash_interface, 1)
                  , m_shell(shell)
{
    m_client = shell->compositor()->launchProcess(LIBEXEC_PATH "/orbital-splash");
}

DesktopShellSplash::~DesktopShellSplash()
{
    qDeleteAll(m_splashes);
}

void DesktopShellSplash::hide()
{
    for (Splash *s: m_splashes) {
        s->fadeOut();
    }
}

void DesktopShellSplash::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &desktop_shell_splash_interface, version, id);
    if (client != m_client->client()) {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "permission to bind desktop_shell_splash denied");
        wl_resource_destroy(resource);
    }

    static const struct desktop_shell_splash_interface implementation = {
        wrapInterface(&DesktopShellSplash::setSplashSurface)
    };

    wl_resource_set_implementation(resource, &implementation, this, nullptr);
    m_resource = resource;
}

void DesktopShellSplash::setSplashSurface(wl_resource *outputResource, wl_resource *surfaceResource)
{
    Surface *surf = Surface::fromResource(surfaceResource);
    Output *out = Output::fromResource(outputResource);

    if (!surf->setRole("desktop_shell_splash", m_resource, DESKTOP_SHELL_SPLASH_ERROR_ROLE)) {
        return;
    }

    int x = out->x(), y = out->y();
    View *view = new View(surf);
    view->setPos(x, y);
    m_shell->compositor()->lockLayer()->addView(view);
    view->setOutput(out);
    view->update();

    Splash *splash = new Splash(this, view);
    surf->setRoleHandler(splash);
    m_splashes.insert(splash);
    connect(out, &QObject::destroyed, splash, &Splash::outputDestroyed);
}

}
