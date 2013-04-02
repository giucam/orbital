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

#include "wayland-desktop-shell-server-protocol.h"

ShellSurface::ShellSurface(Shell *shell, struct weston_surface *surface)
            : m_shell(shell)
            , m_surface(surface)
            , m_type(Type::None)
            , m_pendingType(Type::None)
            , m_parent(nullptr)
{

}

ShellSurface::~ShellSurface()
{
    m_shell->removeShellSurface(this);
}

void ShellSurface::init(uint32_t id)
{
    m_resource.destroy = [](struct wl_resource *resource) { delete static_cast<ShellSurface *>(resource->data); };
    m_resource.object.id = id;
    m_resource.object.interface = &wl_shell_surface_interface;
    m_resource.object.implementation = &m_shell_surface_implementation;
    m_resource.data = this;
}

bool ShellSurface::updateType()
{
    if (m_type != m_pendingType && m_pendingType != Type::None) {
        switch (m_type) {
            case Type::Maximized:
                unsetMaximized();
                break;
            default:
                break;
        }
        m_type = m_pendingType;
        m_pendingType = Type::None;

        switch (m_type) {
            case Type::Maximized:
                m_savedX = x();
                m_savedY = y();
                break;
            case Type::Transient:
                weston_surface_set_position(m_surface, m_parent->geometry.x + m_transient.x, m_parent->geometry.y + m_transient.y);
                break;
            default:
                break;
        }
        return true;
    }
    return false;
}

void ShellSurface::map(int32_t x, int32_t y, int32_t width, int32_t height)
{
//     if (m_type == Type::None) {
//         return;
//     }


//     if (m_type == Type::TopLevel) {
//         weston_surface_set_position(m_surface, x, x);
//     }
    switch (m_type) {
        case Type::Maximized: {
            IRect2D rect = m_shell->windowsArea(m_output);
            x = rect.x;
            y = rect.y;
        }
        default:
            weston_surface_configure(m_surface, x, y, width, height);
    }

    printf("map %d %d %d %d - %d\n",x,y,width,height,m_type);


//     m_surface->geometry.width = width;
//     m_surface->geometry.height = height;
//     weston_surface_geometry_dirty(m_surface);
    if (m_type != Type::None) {
        weston_surface_update_transform(m_surface);
        if (m_type == Type::Maximized) {
            m_surface->output = m_output;
        }
    }
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

void ShellSurface::setAlpha(float alpha)
{
    m_surface->alpha = alpha;
    damage();
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

float ShellSurface::alpha() const
{
    return m_surface->alpha;
}

void ShellSurface::pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{

}

void ShellSurface::unsetMaximized()
{
    weston_surface_set_position(m_surface, m_savedX, m_savedY);
}

// -- Move --

struct MoveGrab : public ShellGrab {
    struct ShellSurface *shsurf;
    struct wl_listener shsurf_destroy_listener;
    wl_fixed_t dx, dy;
};

void ShellSurface::move_grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    ShellGrab *shgrab = container_of(grab, ShellGrab, grab);
    MoveGrab *move = static_cast<MoveGrab *>(shgrab);
    struct wl_pointer *pointer = grab->pointer;
    ShellSurface *shsurf = move->shsurf;
    int dx = wl_fixed_to_int(pointer->x + move->dx);
    int dy = wl_fixed_to_int(pointer->y + move->dy);

    if (!shsurf)
        return;

    struct weston_surface *es = shsurf->m_surface;

    weston_surface_configure(es, dx, dy, es->geometry.width, es->geometry.height);

    weston_compositor_schedule_repaint(es->compositor);
}

void ShellSurface::move_grab_button(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shell_grab = container_of(grab, ShellGrab, grab);
    MoveGrab *move = static_cast<MoveGrab *>(shell_grab);
    struct wl_pointer *pointer = grab->pointer;
    enum wl_pointer_button_state state = (wl_pointer_button_state)state_w;

    if (pointer->button_count == 0 && state == WL_POINTER_BUTTON_STATE_RELEASED) {
        Shell::endGrab(shell_grab);
        delete shell_grab;
        move->shsurf->moveEndSignal(move->shsurf);
    }
}

const struct wl_pointer_grab_interface ShellSurface::m_move_grab_interface = {
    [](struct wl_pointer_grab *grab, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {},
    ShellSurface::move_grab_motion,
    ShellSurface::move_grab_button,
};

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
    dragMove(ws);
}

void ShellSurface::dragMove(struct weston_seat *ws)
{
    MoveGrab *move = new MoveGrab;
    if (!move)
        return;

    move->dx = wl_fixed_from_double(m_surface->geometry.x) - ws->seat.pointer->grab_x;
    move->dy = wl_fixed_from_double(m_surface->geometry.y) - ws->seat.pointer->grab_y;
    move->shsurf = this;
    move->grab.focus = &m_surface->surface;

    m_shell->startGrab(move, &m_move_grab_interface, ws->seat.pointer, DESKTOP_SHELL_CURSOR_MOVE);
    moveStartSignal(this);
}

// -- Resize --

struct ResizeGrab : public ShellGrab {
    struct ShellSurface *shsurf;
    struct wl_listener shsurf_destroy_listener;
    uint32_t edges;
    int32_t width, height;
};

void ShellSurface::resize_grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    ShellGrab *shgrab = container_of(grab, ShellGrab, grab);
    ResizeGrab *resize = static_cast<ResizeGrab *>(shgrab);
    struct wl_pointer *pointer = grab->pointer;
    ShellSurface *shsurf = resize->shsurf;

    if (!shsurf)
        return;

    struct weston_surface *es = shsurf->m_surface;

    wl_fixed_t from_x, from_y;
    wl_fixed_t to_x, to_y;
    weston_surface_from_global_fixed(es, pointer->grab_x, pointer->grab_y, &from_x, &from_y);
    weston_surface_from_global_fixed(es, pointer->x, pointer->y, &to_x, &to_y);

    int32_t width = resize->width;
    if (resize->edges & WL_SHELL_SURFACE_RESIZE_LEFT) {
        width += wl_fixed_to_int(from_x - to_x);
    } else if (resize->edges & WL_SHELL_SURFACE_RESIZE_RIGHT) {
        width += wl_fixed_to_int(to_x - from_x);
    }

    int32_t height = resize->height;
    if (resize->edges & WL_SHELL_SURFACE_RESIZE_TOP) {
        height += wl_fixed_to_int(from_y - to_y);
    } else if (resize->edges & WL_SHELL_SURFACE_RESIZE_BOTTOM) {
        height += wl_fixed_to_int(to_y - from_y);
    }

    shsurf->m_client->send_configure(shsurf->m_surface, resize->edges, width, height);
}

void ShellSurface::resize_grab_button(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shgrab = container_of(grab, ShellGrab, grab);
    ResizeGrab *resize = static_cast<ResizeGrab *>(shgrab);

    if (grab->pointer->button_count == 0 && state_w == WL_POINTER_BUTTON_STATE_RELEASED) {
        Shell::endGrab(resize);
        delete resize;
    }
}

const struct wl_pointer_grab_interface ShellSurface::m_resize_grab_interface = {
    [](struct wl_pointer_grab *grab, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {},
    ShellSurface::resize_grab_motion,
    ShellSurface::resize_grab_button,
};

void ShellSurface::resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource,
                          uint32_t serial, uint32_t edges)
{
    struct weston_seat *ws = static_cast<weston_seat *>(seat_resource->data);

    if (ws->seat.pointer->button_count == 0 || ws->seat.pointer->grab_serial != serial ||
        ws->seat.pointer->focus != &m_surface->surface) {
        return;
    }

    dragResize(ws, edges);
}

void ShellSurface::dragResize(struct weston_seat *ws, uint32_t edges)
{
    ResizeGrab *grab = new ResizeGrab;
    if (!grab)
        return;

    if (edges == 0 || edges > 15 || (edges & 3) == 3 || (edges & 12) == 12) {
        return;
    }

    grab->edges = edges;
    grab->width = m_surface->geometry.width;
    grab->height = m_surface->geometry.height;
    grab->shsurf = this;
    grab->grab.focus = &m_surface->surface;

    m_shell->startGrab(grab, &m_resize_grab_interface, ws->seat.pointer, edges);
}

void ShellSurface::setToplevel(struct wl_client *, struct wl_resource *)
{
printf("top\n");
    m_pendingType = Type::TopLevel;
}

void ShellSurface::setTransient(struct wl_client *client, struct wl_resource *resource,
                  struct wl_resource *parent_resource, int x, int y, uint32_t flags)
{
    m_parent = static_cast<struct weston_surface *>(parent_resource->data);
    m_transient.x = x;
    m_transient.y = y;

    m_pendingType = Type::Transient;

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
    /* get the default output, if the client set it as NULL
    c heck whether the ouput is available */
    if (output_resource) {
        m_output = static_cast<struct weston_output *>(output_resource->data);
    } else if (m_surface->output) {
        m_output = m_surface->output;
    } else {
        m_output = m_shell->getDefaultOutput();
    }

    uint32_t edges = WL_SHELL_SURFACE_RESIZE_TOP | WL_SHELL_SURFACE_RESIZE_LEFT;

    IRect2D rect = m_shell->windowsArea(m_output);
    m_client->send_configure(m_surface, edges, rect.width, rect.height);
    m_pendingType = Type::Maximized;
}

void ShellSurface::setTitle(struct wl_client *client, struct wl_resource *resource, const char *title)
{
    m_title = title;
}

void ShellSurface::setClass(struct wl_client *client, struct wl_resource *resource, const char *className)
{
    m_class = className;
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
