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

#ifndef ORBITAL_DESKTOP_SHELL_LAUNCHER
#define ORBITAL_DESKTOP_SHELL_LAUNCHER

#include <QPointer>

#include "../interface.h"

struct wl_resource;

namespace Orbital {

class Shell;
class KeyBinding;
class Seat;
class LauncherSurface;

class DesktopShellLauncher : public Interface, public Global
{
    Q_OBJECT
public:
    DesktopShellLauncher(Shell *shell);

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    void getLauncherSurface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surfaceResource);
    void toggle(Seat *seat);

    Shell *m_shell;
    QPointer<LauncherSurface> m_surface;
};

}

#endif
