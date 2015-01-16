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

#ifndef ORBITAL_DESKTOP_SHELL_H
#define ORBITAL_DESKTOP_SHELL_H

#include <QPointer>
#include <QTimer>

#include "../interface.h"

struct wl_resource;

namespace Orbital {

class Shell;
class ChildProcess;
class View;
class Pointer;
class DesktopShellSplash;
class Output;
enum class PointerCursor: unsigned int;
struct Listener;

class DesktopShell : public Interface, public Global
{
    Q_OBJECT
public:
    explicit DesktopShell(Shell *shell);
    ~DesktopShell();

    Compositor *compositor() const;
    Shell *shell() const { return m_shell; }
    wl_client *client() const;
    inline wl_resource *resource() const { return m_resource; }

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    void clientExited();
    void setGrabCursor(Pointer *p, PointerCursor c);
    void unsetGrabCursor(Pointer *p);
    void outputCreated(Output *o);
    void pointerMotion(Pointer *p);
    void pingTimeout();
    void session(bool active);

    void setBackground(wl_resource *outputResource, wl_resource *surfaceResource);
    void setPanel(uint32_t id, wl_resource *outputResource, wl_resource *surfaceResource, uint32_t position);
    void setLockSurface(wl_resource *surfaceResource, wl_resource *outputResource);
    void setPopup(uint32_t id, wl_resource *parentResource, wl_resource *surfaceResource, int x, int y);
    void lock();
    void unlock();
    void setGrabSurface(wl_resource *surfaceResource);
    void addKeyBinding(uint32_t id, uint32_t key, uint32_t modifiers);
    void addOverlay(wl_resource *outputResource, wl_resource *surfaceResource);
    void minimizeWindows();
    void restoreWindows();
    void createGrab(uint32_t id);
    void addWorkspace(uint32_t id);
    void selectWorkspace(wl_resource *outputResource, wl_resource *workspaceResource);
    void quit();
    void addTrustedClient(int32_t fd, const char *interface);
    void pong(uint32_t serial);
    void outputLoaded(uint32_t serial);
    void createActiveRegion(uint32_t id, wl_resource *parentResource, int32_t x, int32_t y, int32_t width, int32_t height);

    Shell *m_shell;
    ChildProcess *m_client;
    wl_resource *m_resource;
    QPointer<View> m_grabView;
    DesktopShellSplash *m_splash;
    uint32_t m_loadSerial;
    QTimer m_pingTimer;
    uint32_t m_pingSerial;
    bool m_loaded;
    QHash<Pointer *, PointerCursor> m_grabCursor;
    bool m_lockRequested;
};

}

#endif
