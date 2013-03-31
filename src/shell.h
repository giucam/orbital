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

#ifndef SHELL_H
#define SHELL_H

#include <vector>

// i'd like to not have this here, but it is for the desktop_shell_cursor enum
#include "wayland-desktop-shell-server-protocol.h"

class ShellSurface;
struct ShellGrab;
class Effect;

typedef std::vector<ShellSurface *> ShellSurfaceList;

class Shell;

struct ShellGrab {
    Shell *shell;
    struct wl_pointer_grab grab;
    struct wl_pointer *pointer;
};

class Shell {
public:
    template<class T>
    static Shell *load(struct weston_compositor *ec);
    ~Shell();

    void launchShellProcess();
    ShellSurface *createShellSurface(struct weston_surface *surface, const struct weston_shell_client *client);
    ShellSurface *getShellSurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource);
    void removeShellSurface(ShellSurface *surface);
    static ShellSurface *getShellSurface(struct weston_surface *surf);
    void bindEffect(Effect *effect, uint32_t key, enum weston_keyboard_modifier modifier);

    void configureSurface(ShellSurface *surface, int32_t sx, int32_t sy, int32_t width, int32_t height);

    void moveSurface(ShellSurface *shsurf, struct weston_seat *seat);
    void activateSurface(ShellSurface *shsurf, struct weston_seat *seat);
    void setBackgroundSurface(struct weston_surface *surface, struct weston_output *output);
    void setGrabSurface(struct weston_surface *surface);
    void addPanelSurface(struct weston_surface *surface, struct weston_output *output);

    inline struct weston_compositor *compositor() const { return m_compositor; }

    void startGrab(ShellGrab *grab, const struct wl_pointer_grab_interface *interface,
                   struct wl_pointer *pointer, enum desktop_shell_cursor cursor);
    static void endGrab(ShellGrab *grab);

protected:
    Shell(struct weston_compositor *ec);
    virtual void init();
    inline const struct weston_layer *layer() const { return &m_layer; }
    inline const ShellSurfaceList &surfaces() const { return m_surfaces; }

    struct {
        struct weston_process process;
        struct wl_client *client;
        struct wl_resource *desktop_shell;

        unsigned deathcount;
        uint32_t deathstamp;
    } m_child;

private:
    void bind(struct wl_client *client, uint32_t version, uint32_t id);
    void backgroundConfigure(struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height);
    void panelConfigure(struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height);
    void activateSurface(struct wl_seat *seat, uint32_t time, uint32_t button);

    struct weston_compositor *m_compositor;
    struct weston_layer m_backgroundLayer;
    struct weston_layer m_panelsLayer;
    struct weston_layer m_layer;
    std::vector<Effect *> m_effects;
    ShellSurfaceList m_surfaces;

    struct weston_surface *m_blackSurface;
    struct weston_surface *m_grabSurface;

    static const struct wl_shell_interface shell_implementation;

    static void move_grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    static void move_grab_button(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w);
    static const struct wl_pointer_grab_interface m_move_grab_interface;

    friend class Effect;
};

template<class T>
Shell *Shell::load(struct weston_compositor *ec)
{
    Shell *shell = new T(ec);
    if (shell) {
        shell->init();
    }

    return shell;
}

#endif
