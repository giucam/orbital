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

#ifndef SHELL_SURFACE_H
#define SHELL_SURFACE_H

#include <wayland-server.h>

class Shell;

class ShellSurface {
public:
    enum class Type {
        None,
        TopLevel
    };
    ShellSurface(Shell *shell, struct weston_surface *surface);
    ~ShellSurface();

    void init(uint32_t id);
    void map(int32_t x, int32_t y, int32_t width, int32_t height);

    void addTransform(struct weston_transform *transform);
    void removeTransform(struct weston_transform *transform);
    void damage();

    inline Shell *shell() const { return m_shell; }
    inline struct wl_resource *wl_resource() { return &m_resource; }
    inline const struct wl_resource *wl_resource() const { return &m_resource; }

    bool isMapped() const;
    int32_t x() const;
    int32_t y() const;
    int32_t width() const;
    int32_t height() const;
    inline struct weston_output *output() const { return m_surface->output; }

private:
    Shell *m_shell;
    struct wl_resource m_resource;
    struct weston_surface *m_surface;
    Type m_type;
    Type m_pendingType;

    void pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
    void move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                   uint32_t serial);
    void resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                     uint32_t serial, uint32_t edges);
    void setToplevel(struct wl_client *client, struct wl_resource *resource);
    void setTransient(struct wl_client *client, struct wl_resource *resource,
                                            struct wl_resource *parent_resource, int x, int y, uint32_t flags);
    void setFullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method,
                                             uint32_t framerate, struct wl_resource *output_resource);
    void setPopup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                        uint32_t serial, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags);
    void setMaximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource);
    void setTitle(struct wl_client *client, struct wl_resource *resource, const char *title);
    void setClass(struct wl_client *client, struct wl_resource *resource, const char *className);

    static void shell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
    static void shell_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                   uint32_t serial);
    static void shell_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                     uint32_t serial, uint32_t edges);
    static void shell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource);
    static void shell_surface_set_transient(struct wl_client *client, struct wl_resource *resource,
                                            struct wl_resource *parent_resource, int x, int y, uint32_t flags);
    static void shell_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method,
                                             uint32_t framerate, struct wl_resource *output_resource);
    static void shell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                        uint32_t serial, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags);
    static void shell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource);
    static void shell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title);
    static void shell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *className);
    static const struct wl_shell_surface_interface m_shell_surface_implementation;

    friend class Shell;
};

inline void ShellSurface::shell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    static_cast<ShellSurface *>(resource->data)->pong(client, resource, serial);
}

inline void ShellSurface::shell_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                             uint32_t serial) {
    static_cast<ShellSurface *>(resource->data)->move(client, resource, seat_resource, serial);
}

inline void ShellSurface::shell_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                 uint32_t serial, uint32_t edges) {
    static_cast<ShellSurface *>(resource->data)->resize(client, resource, seat_resource, serial, edges);
}

inline void ShellSurface::shell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource) {
    static_cast<ShellSurface *>(resource->data)->setToplevel(client, resource);
}

inline void ShellSurface::shell_surface_set_transient(struct wl_client *client, struct wl_resource *resource,
                                        struct wl_resource *parent_resource, int x, int y, uint32_t flags) {
    static_cast<ShellSurface *>(resource->data)->setTransient(client, resource, parent_resource, x, y, flags);
}

inline void ShellSurface::shell_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method,
                                         uint32_t framerate, struct wl_resource *output_resource) {
    static_cast<ShellSurface *>(resource->data)->setFullscreen(client, resource, method, framerate, output_resource);
}

inline void ShellSurface::shell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                                    uint32_t serial, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags) {
    static_cast<ShellSurface *>(resource->data)->setPopup(client, resource, seat_resource, serial, parent_resource, x, y, flags);
}

inline void ShellSurface::shell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource,
                                                      struct wl_resource *output_resource) {
    static_cast<ShellSurface *>(resource->data)->setMaximized(client, resource, output_resource);
}

inline void ShellSurface::shell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title) {
    static_cast<ShellSurface *>(resource->data)->setTitle(client, resource, title);
}

inline void ShellSurface::shell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *className) {
    static_cast<ShellSurface *>(resource->data)->setClass(client, resource, className);
}

#endif
