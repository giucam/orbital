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
#include <vector>

#include "interface.h"
#include "stringview.h"

namespace Orbital {

class Compositor;
class Layer;
class Workspace;
class ShellSurface;
class Pointer;
class ButtonBinding;
class KeyBinding;
class AxisBinding;
class Seat;
class Pager;
class Output;
class FocusScope;
class Surface;
enum class PointerCursor: unsigned int;
enum class PointerAxis : unsigned char;

class Shell : public Object
{
    Q_OBJECT
public:
    typedef std::function<void (Pointer *, PointerCursor)> GrabCursorSetter;
    typedef std::function<void (Pointer *)> GrabCursorUnsetter;
    typedef std::function<void ()> LockCallback;
    typedef std::function<void (Seat *)> Action;

    explicit Shell(Compositor *c);
    ~Shell();

    Compositor *compositor() const;
    Pager *pager() const;
    Workspace *createWorkspace();
    ShellSurface *createShellSurface(Surface *surface);
    const std::vector<Workspace *> &workspaces() const;
    const std::vector<ShellSurface *> &surfaces() const;
    Output *selectPrimaryOutput(Seat *seat = nullptr);

    FocusScope *lockFocusScope() const { return m_lockScope; }
    FocusScope *appsFocusScope() const { return m_appsScope; }

    void lock(const LockCallback &callback = nullptr);
    void unlock();
    bool isLocked() const;

    bool snapPos(Output *out, QPointF &p, int margin = -1) const;

    void setGrabCursor(Pointer *pointer, PointerCursor c);
    void unsetGrabCursor(Pointer *pointer);
    void configure(ShellSurface *shsurf);
    bool isSurfaceActive(ShellSurface *shsurf) const;

    void setGrabCursorSetter(GrabCursorSetter s);
    void setGrabCursorUnsetter(GrabCursorUnsetter s);

    void addAction(StringView name, const Action &action);

    class ActionList {
    public:
        class iterator {
        public:
            StringView name();
            Action *action();

            iterator &operator++() { ++id; return *this; }
            bool operator!=(const iterator &o) const { return id != o.id || shell != o.shell; }

        private:
            iterator(int i, Shell *s) : id(i), shell(s) {}
            int id;
            Shell *shell;
            friend ActionList;
        };

        iterator begin();
        iterator end();

    private:
        ActionList(Shell *shell);
        Shell *shell;
        friend Shell;
    };
    ActionList actions() { return ActionList(this); }

signals:
    void aboutToLock();
    void locked();
    void actionAdded(StringView name, Action *action);

private:
    void giveFocus(Seat *s);
    void raise(Seat *s);
    void moveSurface(Seat *s);
    void killSurface(Seat *s);
    void nextWs(Seat *s);
    void prevWs(Seat *s);
    void setAlpha(Seat *s, uint32_t time, PointerAxis axis, double value);
    void initEnvironment();
    void autostartClients();

    Compositor *m_compositor;
    std::vector<Workspace *> m_workspaces;
    std::vector<ShellSurface *> m_surfaces;
    GrabCursorSetter m_grabCursorSetter;
    GrabCursorUnsetter m_grabCursorUnsetter;
    ButtonBinding *m_focusBinding;
    ButtonBinding *m_raiseBinding;
    ButtonBinding *m_moveBinding;
    KeyBinding *m_killBinding;
    AxisBinding *m_alphaBinding;
    Pager *m_pager;
    bool m_locked;
    FocusScope *m_lockScope;
    FocusScope *m_appsScope;
    std::vector<std::pair<std::string, Action>> m_actions;
};

}

#endif
