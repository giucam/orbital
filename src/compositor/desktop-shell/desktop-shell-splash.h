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

#ifndef ORBITAL_DESKTOP_SHELL_SPLASH
#define ORBITAL_DESKTOP_SHELL_SPLASH

#include <QSet>

#include "interface.h"

struct wl_resource;

namespace Orbital {

class Shell;
class ChildProcess;
class Splash;

class DesktopShellSplash : public Interface, public Global
{
    Q_OBJECT
public:
    DesktopShellSplash(Shell *shell);

    void hide();

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    void setSplashSurface(wl_resource *outputResource, wl_resource *surfaceResource);

    Shell *m_shell;
    ChildProcess *m_client;
    wl_resource *m_resource;
    QSet<Splash *> m_splashes;

    friend Splash;
};

}

#endif
