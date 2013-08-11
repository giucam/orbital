/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef DESKTOP_SHELL_H
#define DESKTOP_SHELL_H

#include "shell.h"

class InputPanel;

class DesktopShell : public Shell {
public:
    DesktopShell(struct weston_compositor *ec);

protected:
    virtual void init();
    virtual void setGrabCursor(uint32_t);
    virtual void setBusyCursor(ShellSurface *shsurf, struct weston_seat *seat) override;
    virtual void endBusyCursor(struct weston_seat *seat) override;

private:
    void sendInitEvents();
    void workspaceAdded(Workspace *ws);
    void bind(struct wl_client *client, uint32_t version, uint32_t id);
    void unbind(struct wl_resource *resource);
    void moveBinding(struct weston_seat *seat, uint32_t time, uint32_t button);

    void setBackground(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource,
                                             struct wl_resource *surface_resource);
    void setPanel(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource,
                                        struct wl_resource *surface_resource);
    void setLockSurface(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface_resource);
    void unlock(struct wl_client *client, struct wl_resource *resource);
    void setGrabSurface(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface_resource);
    void desktopReady(struct wl_client *client, struct wl_resource *resource);
    void addKeyBinding(struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t key, uint32_t modifiers);
    void addOverlay(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource);
    void requestFocus(wl_client *client, wl_resource *resource, wl_resource *surface_resource);
    void minimizeWindows(wl_client *client, wl_resource *resource);
    void restoreWindows(wl_client *client, wl_resource *resource);
    void createGrab(wl_client *client, wl_resource *resource, uint32_t id);
    void addWorkspace(wl_client *client, wl_resource *resource);
    void selectWorkspace(wl_client *client, wl_resource *resource, wl_resource *workspace_resource);
    void quit(wl_client *client, wl_resource *resource);

    static void desktop_shell_set_background(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource,
                                             struct wl_resource *surface_resource);
    static void desktop_shell_set_panel(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource,
                                        struct wl_resource *surface_resource);
    static void desktop_shell_set_lock_surface(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface_resource);
    static void desktop_shell_unlock(struct wl_client *client, struct wl_resource *resource);
    static void desktop_shell_set_grab_surface(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface_resource);
    static void desktop_shell_desktop_ready(struct wl_client *client, struct wl_resource *resource);
    static void desktop_shell_add_key_binding(struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t key, uint32_t modifiers);
    static const struct desktop_shell_interface m_desktop_shell_implementation;
    static void desktop_shell_add_overlay(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource);
    static void desktop_shell_request_focus(wl_client *client, wl_resource *resource, wl_resource *surface_resource);
    static void desktop_shell_minimize_windows(wl_client *client, wl_resource *resource);
    static void desktop_shell_restore_windows(wl_client *client, wl_resource *resource);
    static void desktop_shell_create_grab(wl_client *client, wl_resource *resource, uint32_t id);
    static void desktop_shell_add_workspace(wl_client *client, wl_resource *resource);
    static void desktop_shell_select_workspace(wl_client *client, wl_resource *resource, wl_resource *workspace_resource);
    static void desktop_shell_quit(wl_client *client, wl_resource *resource);

    InputPanel *m_inputPanel;
};

#define _this static_cast<DesktopShell *>(wl_resource_get_user_data(resource))
inline void DesktopShell::desktop_shell_set_background(struct wl_client *client, struct wl_resource *resource,
                                                       struct wl_resource *output_resource, struct wl_resource *surface_resource) {
    _this->setBackground(client, resource, output_resource, surface_resource);
}

inline void DesktopShell::desktop_shell_set_panel(struct wl_client *client, struct wl_resource *resource,
                                                  struct wl_resource *output_resource, struct wl_resource *surface_resource) {
    _this->setPanel(client, resource, output_resource, surface_resource);
}

inline void DesktopShell::desktop_shell_set_lock_surface(struct wl_client *client, struct wl_resource *resource,
                                                         struct wl_resource *surface_resource) {
    _this->setLockSurface(client, resource, surface_resource);
}

inline void DesktopShell::desktop_shell_unlock(struct wl_client *client, struct wl_resource *resource) {
    _this->unlock(client, resource);
}

inline void DesktopShell::desktop_shell_set_grab_surface(struct wl_client *client, struct wl_resource *resource,
                                                         struct wl_resource *surface_resource) {
    _this->setGrabSurface(client, resource, surface_resource);
}

inline void DesktopShell::desktop_shell_desktop_ready(wl_client *client, wl_resource *resource) {
    _this->desktopReady(client, resource);
}

inline void DesktopShell::desktop_shell_add_key_binding(struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t key, uint32_t modifiers) {
    _this->addKeyBinding(client, resource, id, key, modifiers);
}

inline void DesktopShell::desktop_shell_add_overlay(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource) {
    _this->addOverlay(client, resource, output_resource, surface_resource);
}

inline void DesktopShell::desktop_shell_request_focus(wl_client *client, wl_resource *resource, wl_resource *surface_resource) {
    _this->requestFocus(client, resource, surface_resource);
}

inline void DesktopShell::desktop_shell_minimize_windows(wl_client *client, wl_resource *resource) {
    _this->minimizeWindows(client, resource);
}

inline void DesktopShell::desktop_shell_restore_windows(wl_client *client, wl_resource *resource) {
    _this->restoreWindows(client, resource);
}

inline void DesktopShell::desktop_shell_create_grab(wl_client *client, wl_resource *resource, uint32_t id) {
    _this->createGrab(client, resource, id);
}

inline void DesktopShell::desktop_shell_add_workspace(wl_client *client, wl_resource *resource)
{
    _this->addWorkspace(client, resource);
}

inline void DesktopShell::desktop_shell_select_workspace(wl_client *client, wl_resource *resource, wl_resource *workspace_resource)
{
    _this->selectWorkspace(client, resource, workspace_resource);
}

inline void DesktopShell::desktop_shell_quit(wl_client *client, wl_resource *resource) {
    _this->quit(client, resource);
}
#undef _this

#endif
