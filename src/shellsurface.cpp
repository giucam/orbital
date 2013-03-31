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

#include <stdio.h>

#include <weston/compositor.h>

#include "shellsurface.h"
#include "shell.h"

ShellSurface::ShellSurface(Shell *shell, struct weston_surface *surface)
            : m_shell(shell)
            , m_surface(surface)
            , m_type(Type::None)
            , m_pendingType(Type::None)
{

}

void ShellSurface::init(uint32_t id)
{
    m_resource.destroy = [](struct wl_resource *resource) { delete static_cast<ShellSurface *>(resource->data); };
    m_resource.object.id = id;
    m_resource.object.interface = &wl_shell_surface_interface;
    m_resource.object.implementation = &m_shell_surface_implementation;
    m_resource.data = this;
}

void ShellSurface::map(int32_t x, int32_t y, int32_t width, int32_t height)
{
//     if (m_type == Type::None) {
//         return;
//     }


//     if (m_type == Type::TopLevel) {
//         weston_surface_set_position(m_surface, x, x);
//     }
printf("map %d %d %d %d\n",x,y,width,height);
    weston_surface_configure(m_surface, x, y, width, height);
//     m_surface->geometry.width = width;
//     m_surface->geometry.height = height;
//     weston_surface_geometry_dirty(m_surface);
    weston_surface_update_transform(m_surface);
}

void ShellSurface::addTransform(struct weston_transform *transform)
{
    removeTransform(transform);
    wl_list_insert(&m_surface->geometry.transformation_list, &transform->link);

    damage();
}

void ShellSurface::removeTransform(struct weston_transform *transform)
{
    if (wl_list_empty(&transform->link)) {
        return;
    }

    wl_list_remove(&transform->link);
    wl_list_init(&transform->link);

    damage();
}

void ShellSurface::damage()
{
    weston_surface_geometry_dirty(m_surface);
    weston_surface_damage(m_surface);
}

bool ShellSurface::isMapped() const
{
    return weston_surface_is_mapped(m_surface);
}

int32_t ShellSurface::x() const
{
    return m_surface->geometry.x;
}

int32_t ShellSurface::y() const
{
    return m_surface->geometry.y;
}

int32_t ShellSurface::width() const
{
    return m_surface->geometry.width;
}

int32_t ShellSurface::height() const
{
    return m_surface->geometry.height;
}

void ShellSurface::pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{

}

void ShellSurface::move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
          uint32_t serial)
{
    struct weston_seat *ws = static_cast<weston_seat *>(seat_resource->data);

    if (ws->seat.pointer->button_count == 0 || ws->seat.pointer->grab_serial != serial ||
        ws->seat.pointer->focus != &m_surface->surface) {
        return;
    }

//     if (shsurf->type == SHELL_SURFACE_FULLSCREEN)
//         return 0;

    m_shell->moveSurface(this, ws);
}

void ShellSurface::resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
            uint32_t serial, uint32_t edges)
{

}

void ShellSurface::setToplevel(struct wl_client *, struct wl_resource *)
{
printf("top\n");
    m_pendingType = Type::TopLevel;
}

void ShellSurface::setTransient(struct wl_client *client, struct wl_resource *resource,
                  struct wl_resource *parent_resource, int x, int y, uint32_t flags)
{

}

void ShellSurface::setFullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method,
                   uint32_t framerate, struct wl_resource *output_resource)
{

}

void ShellSurface::setPopup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
              uint32_t serial, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags)
{

}

void ShellSurface::setMaximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource)
{

}

void ShellSurface::setTitle(struct wl_client *client, struct wl_resource *resource, const char *title)
{

}

void ShellSurface::setClass(struct wl_client *client, struct wl_resource *resource, const char *className)
{

}

const struct wl_shell_surface_interface ShellSurface::m_shell_surface_implementation = {
    ShellSurface::shell_surface_pong,
    ShellSurface::shell_surface_move,
    ShellSurface::shell_surface_resize,
    ShellSurface::shell_surface_set_toplevel,
    ShellSurface::shell_surface_set_transient,
    ShellSurface::shell_surface_set_fullscreen,
    ShellSurface::shell_surface_set_popup,
    ShellSurface::shell_surface_set_maximized,
    ShellSurface::shell_surface_set_title,
    ShellSurface::shell_surface_set_class
};
