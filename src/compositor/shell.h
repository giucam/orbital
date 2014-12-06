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

#ifndef ORBITAL_SHELL_H
#define ORBITAL_SHELL_H

#include <functional>

#include <QHash>

#include "interface.h"

struct weston_surface;

namespace Orbital {

class Compositor;
class Layer;
class Workspace;
class ShellSurface;
class Pointer;
class ButtonBinding;
class KeyBinding;
class Seat;
class Pager;
class Output;
class Client;
enum class PointerCursor: unsigned int;

class Shell : public Object
{
    Q_OBJECT
public:
    typedef std::function<void (Pointer *, PointerCursor)> GrabCursorSetter;

    explicit Shell(Compositor *c);
    ~Shell();

    Compositor *compositor() const;
    Pager *pager() const;
    Workspace *createWorkspace();
    ShellSurface *createShellSurface(weston_surface *surface);
    QList<Workspace *> workspaces() const;
    QList<ShellSurface *> surfaces() const;
    Output *selectPrimaryOutput(Seat *seat = nullptr);

    void lock();
    void unlock();

    void setGrabCursor(Pointer *pointer, PointerCursor c);
    void configure(ShellSurface *shsurf);

    void setGrabCursorSetter(GrabCursorSetter s);

    void addTrustedClient(const QString &interface, wl_client *c);
    bool isClientTrusted(const QString &interface, wl_client *c) const;
private:
    void giveFocus(Seat *s);
    void raise(Seat *s);
    void moveSurface(Seat *s);
    void killSurface(Seat *s);
    void nextWs(Seat *s);
    void prevWs(Seat *s);
    void initEnvironment();
    void autostartClients();

    Compositor *m_compositor;
    QList<Workspace *> m_workspaces;
    QList<ShellSurface *> m_surfaces;
    GrabCursorSetter m_grabCursorSetter;
    ButtonBinding *m_focusBinding;
    ButtonBinding *m_raiseBinding;
    ButtonBinding *m_moveBinding;
    KeyBinding *m_killBinding;
    KeyBinding *m_nextWsBinding;
    KeyBinding *m_prevWsBinding;
    Pager *m_pager;
    QHash<QString, QList<Client *>> m_trustedClients;
};

}

#endif
