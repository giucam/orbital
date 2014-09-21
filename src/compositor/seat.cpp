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

#include <linux/input.h>

#include <QDebug>

#include <weston/compositor.h>

#include "seat.h"
#include "compositor.h"
#include "shellsurface.h"
#include "view.h"
#include "shell.h"
#include "global.h"

namespace Orbital {

struct Grab {
    weston_pointer_grab base;
    PointerGrab *parent;
};

struct Listener {
    wl_listener listener;
    Seat *seat;
};

static void seatDestroyed(wl_listener *listener, void *data)
{
    delete reinterpret_cast<Listener *>(listener)->seat;
}

Seat::Seat(Compositor *c, weston_seat *s)
    : m_compositor(c)
    , m_seat(s)
    , m_listener(new Listener)
    , m_pointer(s->pointer ? new Pointer(this, s->pointer) : nullptr)
    , m_activeSurface(nullptr)
    , m_popupGrab(nullptr)
{
    m_listener->listener.notify = seatDestroyed;
    m_listener->seat = this;
    wl_signal_add(&s->destroy_signal, &m_listener->listener);
}

Seat::~Seat()
{
    delete m_listener;
}

Compositor *Seat::compositor() const
{
    return m_compositor;
}

Pointer *Seat::pointer() const
{
    return m_pointer;
}

Surface *Seat::activate(Surface *surface)
{
    bool isNull = !surface;
    if (surface) {
        surface = surface->isActivable() ? surface->activate(this) : nullptr;
    }

    if ((!surface && !isNull) || surface == m_activeSurface) {
        return m_activeSurface;
    }

    weston_surface_activate(surface->surface(), m_seat);

    if (m_activeSurface) {
        emit m_activeSurface->deactivated(this);
    }
    m_activeSurface = surface;
    if (m_activeSurface) {
        emit m_activeSurface->activated(this);
        QObject::connect(m_activeSurface, &QObject::destroyed, [this](QObject *o) {
            if (m_activeSurface == static_cast<Surface *>(o)) {
                m_activeSurface = nullptr;
            }
        });
    }
    return m_activeSurface;
}

class Seat::PopupGrab : public PointerGrab
{
public:
    PopupGrab(Seat *s, wl_client *c)
        : seat(s)
        , client(c)
    {
        start(s);

        /* We must make sure here that this popup was opened after
         * a mouse press, and not just by moving around with other
         * popups already open. */
        if (pointer()->buttonCount() == 0) {
            initialUp = false;
        }
    }
    void focus() override
    {
        double sx, sy;
        View *view = pointer()->pickView(&sx, &sy);

        if (view && view->client() ==client) {
            pointer()->setFocus(view, sx, sy);
        } else {
            pointer()->setFocus(nullptr, 0, 0);
        }
    }
    void motion(uint32_t time, double x, double y) override
    {
        pointer()->move(x, y);
        pointer()->sendMotion(time);
    }
    void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
    {
        if (pointer()->focus()) {
            pointer()->sendButton(time, button, state);
        } else if (state == Pointer::ButtonState::Released && (initialUp || time - pointer()->grabTime() > 500)) {
            end();
        }

        if (state == Pointer::ButtonState::Released) {
            initialUp = true;
        }
    }
    void ended() override
    {
        for (ShellSurface *shsurf: surfaces) {
            shsurf->sendPopupDone();
        }
        seat->m_popupGrab = nullptr;
        delete this;
    }

    Seat *seat;
    wl_client *client;
    QSet<ShellSurface *> surfaces;
    bool initialUp;
};

void Seat::grabPopup(ShellSurface *surf)
{
    if (!m_popupGrab) {
        m_popupGrab = new PopupGrab(this, surf->client());
    }
    m_popupGrab->surfaces.insert(surf);
}

void Seat::ungrabPopup(ShellSurface *shsurf)
{
    if (m_popupGrab) {
        m_popupGrab->surfaces.remove(shsurf);
        if (m_popupGrab->surfaces.isEmpty()) {
            m_popupGrab->end();
        }
    }
}

Seat *Seat::fromSeat(weston_seat *s)
{
    wl_listener *listener = wl_signal_get(&s->destroy_signal, seatDestroyed);
    if (!listener) {
        return new Seat(Compositor::fromCompositor(s->compositor), s);
    }

    return reinterpret_cast<Listener *>(listener)->seat;
}

Seat *Seat::fromResource(wl_resource *res)
{
    weston_seat *s = static_cast<weston_seat *>(wl_resource_get_user_data(res));
    return fromSeat(s);
}



// -- Pointer

Pointer::Pointer(Seat *seat, weston_pointer *p)
       : m_seat(seat)
       , m_pointer(p)
       , m_defaultGrab(nullptr)
{
}

Pointer::~Pointer()
{
    delete m_defaultGrab;
}

View *Pointer::pickView(double *vx, double *vy) const
{
    weston_view *view;
    wl_list_for_each(view, &m_seat->compositor()->m_compositor->view_list, link) {
        View *v = View::fromView(view);
        if (!v) {
            continue;
        }

        if (View *target = v->dispatchPointerEvent(this, m_pointer->x, m_pointer->y)) {
            if (vx || vy) {
                QPointF pos = target->mapFromGlobal(QPointF(x(), y()));
                if (vx) *vx = pos.x();
                if (vy) *vy = pos.y();
            }
            return target;
        }
    }
    return nullptr;
}

void Pointer::setDefaultGrab(PointerGrab *grab)
{
    if (!isGrabActive()) {
        m_defaultGrab = grab;
        m_defaultGrab->start(m_seat);
    } else {
        m_defaultGrab = grab;
    }
}

void Pointer::setFocus(View *view)
{
    setFocus(view, view->mapFromGlobal(QPointF(x(), y())));
}

void Pointer::setFocus(View *view, double x, double y)
{
    wl_fixed_t fx = wl_fixed_from_double(x);
    wl_fixed_t fy = wl_fixed_from_double(y);
    weston_pointer_set_focus(m_pointer, view ? view->m_view : nullptr, fx, fy);
}

View *Pointer::focus() const
{
    weston_view *view = m_pointer->focus;
    if (view) {
        View *v = View::fromView(view);
        if (v) {
            return v;
        }
    }
    return nullptr;
}

void Pointer::move(double x, double y)
{
    weston_pointer_move(m_pointer, wl_fixed_from_double(x), wl_fixed_from_double(y));

    weston_view *view;
    wl_list_for_each(view, &m_seat->compositor()->m_compositor->view_list, link) {
        View *v = View::fromView(view);
        if (v && v->dispatchPointerEvent(this, m_pointer->x, m_pointer->y)) {
            return;
        }
    }
}

void Pointer::sendMotion(uint32_t time)
{
    if (!m_pointer->focus) {
        return;
    }

    wl_resource *resource;
    wl_fixed_t sx, sy;
    weston_view_from_global_fixed(m_pointer->focus, m_pointer->x, m_pointer->y, &sx, &sy);
    wl_resource_for_each(resource, &m_pointer->focus_resource_list) {
        wl_pointer_send_motion(resource, time, sx, sy);
    }
}

void Pointer::sendButton(uint32_t time, PointerButton button, Pointer::ButtonState state)
{
    wl_resource *resource;
    uint32_t serial = m_seat->compositor()->nextSerial();
    wl_resource_for_each(resource, &m_pointer->focus_resource_list) {
        wl_pointer_send_button(resource, serial, time, pointerButtonToRaw(button), (uint32_t)state);
    }
}

int Pointer::buttonCount() const
{
    return m_pointer->button_count;
}

double Pointer::x() const
{
    return wl_fixed_to_double(m_pointer->x);
}

double Pointer::y() const
{
    return wl_fixed_to_double(m_pointer->y);
}

bool Pointer::isGrabActive() const
{
    return m_pointer->grab != &m_pointer->default_grab && (!m_defaultGrab || m_pointer->grab != &m_defaultGrab->m_grab->base);
}

uint32_t Pointer::grabSerial() const
{
    return m_pointer->grab_serial;
}

uint32_t Pointer::grabTime() const
{
    return m_pointer->grab_time;
}

PointerButton Pointer::grabButton() const
{
    return rawToPointerButton(m_pointer->grab_button);
}

QPointF Pointer::grabPos() const
{
    return QPointF(wl_fixed_to_double(m_pointer->grab_x), wl_fixed_to_double(m_pointer->grab_y));
}

// -- PointerGrab

static PointerGrab *fromGrab(weston_pointer_grab *grab)
{
    if (grab == &grab->pointer->default_grab) {
        return nullptr;
    }

    Grab *wrapper = reinterpret_cast<Grab *>(grab);
    return wrapper->parent;
}

PointerGrab::PointerGrab()
           : m_seat(nullptr)
           , m_grab(new Grab)
{
    static const weston_pointer_grab_interface grabInterface = {
        [](weston_pointer_grab *base)                                                 { fromGrab(base)->focus(); },
        [](weston_pointer_grab *base, uint32_t time, wl_fixed_t x, wl_fixed_t y)      {
            fromGrab(base)->motion(time, wl_fixed_to_double(x), wl_fixed_to_double(y));
        },
        [](weston_pointer_grab *base, uint32_t time, uint32_t button, uint32_t state) {
            fromGrab(base)->button(time, rawToPointerButton(button), (Pointer::ButtonState)state);
        },
        [](weston_pointer_grab *base)                                                 { fromGrab(base)->cancel(); }
    };

    m_grab->base.interface = &grabInterface;
    m_grab->parent = this;
}

PointerGrab::~PointerGrab()
{
    end();
}

void PointerGrab::start(Seat *seat)
{
    end();
    if (PointerGrab *grab = fromGrab(seat->pointer()->m_pointer->grab)) {
        grab->end();
    }

    m_seat = seat;
    weston_pointer_start_grab(m_seat->pointer()->m_pointer, &m_grab->base);
}

void PointerGrab::start(Seat *seat, PointerCursor cursor)
{
    start(seat);
    setCursor(cursor);
}

void PointerGrab::end()
{
    if (m_seat) {
        PointerGrab *defaultGrab = pointer()->m_defaultGrab;
        if (defaultGrab != this) {
            weston_pointer_end_grab(m_seat->pointer()->m_pointer);
            m_seat = nullptr;
            ended();
            if (defaultGrab) {
                defaultGrab->start(defaultGrab->m_seat);
            }
        }
    }
}

Pointer *PointerGrab::pointer() const
{
    return m_seat->pointer();
}

void PointerGrab::setCursor(PointerCursor cursor)
{
    m_seat->compositor()->shell()->setGrabCursor(pointer(), cursor);
}

uint32_t pointerButtonToRaw(PointerButton button)
{
    return (uint32_t)button + 0x110;
}

PointerButton rawToPointerButton(uint32_t button)
{
    if (button < pointerButtonToRaw(PointerButton::Left) || button > pointerButtonToRaw(PointerButton::Extra2)) {
        qWarning("Unknown mouse button: %d\n", button);
        return (PointerButton)button;
    }

    return (PointerButton)(button - 0x110);
}

}
