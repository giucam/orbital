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

#include <compositor.h>

#include "seat.h"
#include "compositor.h"
#include "shellsurface.h"
#include "view.h"
#include "shell.h"
#include "global.h"
#include "workspace.h"
#include "output.h"

namespace Orbital {

struct PointerGrab::Grab {
    weston_pointer_grab base;
    PointerGrab *parent;
};

struct Listener {
    wl_listener listener;
    wl_listener capsListener;
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
    m_listener->seat = this;
    m_listener->listener.notify = seatDestroyed;
    wl_signal_add(&s->destroy_signal, &m_listener->listener);
    m_listener->capsListener.notify = [](wl_listener *listener, void *) {
        container_of(listener, Listener, capsListener)->seat->capsUpdated();
    };
    wl_signal_add(&s->updated_caps_signal, &m_listener->capsListener);
}

Seat::~Seat()
{
    delete m_listener;
    if (m_activeSurface) {
        emit m_activeSurface->deactivated(this);
    }
}

Compositor *Seat::compositor() const
{
    return m_compositor;
}

Pointer *Seat::pointer() const
{
    if (m_pointer) {
        return m_pointer;
    }

    if (m_seat->pointer) {
        Seat *that = const_cast<Seat *>(this);
        that->m_pointer = new Pointer(that, m_seat->pointer);
    }
    return m_pointer;
}

Surface *Seat::activate(Surface *surface)
{
    bool isNull = !surface;
    if (surface) {
        surface = surface->isActivable() ? surface->activate(this) : nullptr;
    }

    if (surface || isNull) {
        weston_surface_activate(surface ? surface->surface() : nullptr, m_seat);
    }

    if ((!surface && !isNull) || surface == m_activeSurface) {
        return m_activeSurface;
    }

    if (m_activeSurface) {
        emit m_activeSurface->deactivated(this);
    }
    m_activeSurface = surface;
    if (m_activeSurface) {
        emit m_activeSurface->activated(this);
        connect(m_activeSurface, &Surface::unmapped, this, &Seat::deactivateSurface);

        m_activeSurfaces.removeOne(surface);
        m_activeSurfaces.prepend(surface);
    }
    return m_activeSurface;
}

void Seat::activate(Workspace *ws)
{
    Surface *surface = nullptr;
    for (Surface *s: m_activeSurfaces) {
        if (s->workspaceMask() & ws->mask() || s->workspaceMask() == -1) {
            surface = s;
            break;
        }
    }
    activate(surface);
}

void Seat::deactivateSurface()
{
    Surface *surface = static_cast<Surface *>(sender());

    m_activeSurfaces.removeOne(surface);
    if (surface == m_activeSurface) {
        m_activeSurface->deactivated(this);
        m_activeSurface = nullptr;

        if (m_activeSurfaces.isEmpty()) {
            return;
        }

        int mask = 0;
        for (Output *out: m_compositor->outputs()) {
            mask |= out->currentWorkspace()->mask();
        }
        for (Surface *surf: m_activeSurfaces) {
            if (surf->workspaceMask() & mask) {
                activate(surf);
                break;
            }
        }
    }
}

void Seat::capsUpdated()
{
    // If we now have a keyboard we want to set the focused surface
    if (m_activeSurface && m_seat->keyboard) {
        activate(m_activeSurface);
    }
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
{
    m_hotSpotState.lastTime = 0;
    m_hotSpotState.enterHotZone = 0;

    QObject::connect(m_seat->compositor(), &Compositor::outputRemoved, [this](Output *o) {
        m_defaultGrab.outputs.remove(o);
    });
}

Pointer::~Pointer()
{
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

void Pointer::setFocus(View *view)
{
    setFocus(view, view ? view->mapFromGlobal(QPointF(x(), y())) : QPointF());
}

void Pointer::setFocus(View *view, double x, double y)
{
    wl_fixed_t fx = wl_fixed_from_double(x);
    wl_fixed_t fy = wl_fixed_from_double(y);
    weston_pointer_set_focus(m_pointer, view ? view->m_view : nullptr, fx, fy);
}

void Pointer::setFocusFixed(View *view, wl_fixed_t x, wl_fixed_t y)
{
    weston_pointer_set_focus(m_pointer, view ? view->m_view : nullptr, x, y);
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
    if (focus()) {
        QPointF pos = focus()->mapFromGlobal(QPointF(x, y));
        m_pointer->sx = wl_fixed_from_double(pos.x());
        m_pointer->sy = wl_fixed_from_double(pos.y());
    }

    weston_pointer_move(m_pointer, wl_fixed_from_double(x), wl_fixed_from_double(y));

    weston_view *view;
    wl_list_for_each(view, &m_seat->compositor()->m_compositor->view_list, link) {
        View *v = View::fromView(view);
        if (v && v->dispatchPointerEvent(this, m_pointer->x, m_pointer->y)) {
            break;
        }
    }
    emit m_seat->pointerMotion(this);
}

void Pointer::sendMotion(uint32_t time)
{
    if (!m_pointer->focus) {
        return;
    }

    wl_resource *resource;
    weston_view_from_global_fixed(m_pointer->focus, m_pointer->x, m_pointer->y, &m_pointer->sx, &m_pointer->sy);
    wl_resource_for_each(resource, &m_pointer->focus_resource_list) {
        wl_pointer_send_motion(resource, time, m_pointer->sx, m_pointer->sy);
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
    return m_pointer->grab != &m_pointer->default_grab;
}

PointerGrab *Pointer::activeGrab() const
{
    return PointerGrab::fromGrab(m_pointer->grab);
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

void Pointer::defaultGrabFocus()
{
    if (buttonCount() > 0) {
        return;
    }

    QPoint p(x(), y());
    foreach (Output *out, m_defaultGrab.outputs) {
        if (!out->geometry().contains(p)) {
            emit out->pointerLeave(this);
            m_defaultGrab.outputs.remove(out);
        }
    }
    foreach (Output *out, m_seat->compositor()->outputs()) {
        if (!m_defaultGrab.outputs.contains(out) && out->geometry().contains(p)) {
            emit out->pointerEnter(this);
            m_defaultGrab.outputs << out;
        }
    }

    double dx, dy;
    View *view = pickView(&dx, &dy);
    wl_fixed_t sx = wl_fixed_from_double(dx);
    wl_fixed_t sy = wl_fixed_from_double(dy);

    if (focus() != view || m_pointer->sx != sx || m_pointer->sy != sy) {
        setFocusFixed(view, sx, sy);
    }
}

void Pointer::defaultGrabMotion(uint32_t time, double x, double y)
{
    move(x, y);
    sendMotion(time);
    handleMotionBinding(time, x, y);
}

void Pointer::handleMotionBinding(uint32_t time, double x, double y)
{
    if (time - m_hotSpotState.lastTime < 1000) {
        return;
    }

    Output *out = nullptr;
    foreach (Output *o, m_seat->compositor()->outputs()) {
        if (!out || o->contains(x, y)) {
            out = o;
        }
    }

    const int pushTime = 150;
    PointerHotSpot hs;
    bool inHs = true;
    QRect geom = out->geometry();
    if (x <= geom.x() && y <= geom.y()) {
        hs = PointerHotSpot::TopLeftCorner;
    } else if (x >= geom.right() - 1 && y <= geom.y()) {
        hs = PointerHotSpot::TopRightCorner;
    } else if (x <= geom.x() && y >= geom.bottom() - 1) {
        hs = PointerHotSpot::BottomLeftCorner;
    } else if (x >= geom.right() - 1 && y >= geom.bottom() - 1) {
        hs = PointerHotSpot::BottomRightCorner;
    } else {
        inHs = false;
        m_hotSpotState.enterHotZone = 0;
    }

    if (inHs) {
        if (m_hotSpotState.enterHotZone == 0) {
            m_hotSpotState.enterHotZone = time;
        } else if (time - m_hotSpotState.enterHotZone > pushTime) {
            m_hotSpotState.lastTime = time;
            m_seat->compositor()->handleHotSpot(m_seat, time, hs);
        }
    }
}

void Pointer::defaultGrabButton(uint32_t time, uint32_t btn, uint32_t state)
{
    PointerButton button = rawToPointerButton(btn);
    Pointer::ButtonState st = (Pointer::ButtonState)(state);
    sendButton(time, button, st);

    if (buttonCount() == 0 && st == Pointer::ButtonState::Released) {
        defaultGrabFocus();
    }
}

// -- PointerGrab

PointerGrab *PointerGrab::fromGrab(weston_pointer_grab *grab)
{
    if (grab == &grab->pointer->default_grab) {
        return nullptr;
    }

    PointerGrab::Grab *wrapper = reinterpret_cast<PointerGrab::Grab *>(grab);
    return wrapper->parent;
}

PointerGrab::PointerGrab()
           : m_seat(nullptr)
           , m_grab(new Grab)
{
    static const weston_pointer_grab_interface grabInterface = {
        [](weston_pointer_grab *base)                                                 { fromGrab(base)->focus(); },
        [](weston_pointer_grab *base, uint32_t time, wl_fixed_t x, wl_fixed_t y)      {
            PointerGrab *grab = fromGrab(base);
            Pointer *p = grab->pointer();
            double dx = wl_fixed_to_double(x);
            double dy = wl_fixed_to_double(y);
            grab->motion(time, dx, dy);
            p->handleMotionBinding(time, dx, dy);
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
        weston_pointer_end_grab(m_seat->pointer()->m_pointer);
        m_seat->compositor()->shell()->unsetGrabCursor(pointer());
        m_seat = nullptr;
        ended();
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
