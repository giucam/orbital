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
#include "focusscope.h"
#include "layer.h"

namespace Orbital {


Keymap::Keymap(const Maybe<QString> &layout, const Maybe<QString> &options)
      : m_layout(layout)
      , m_options(options)
{
}

void Keymap::fill(const Keymap &other)
{
    if (!m_layout) {
        m_layout = other.m_layout;
    }
    if (!m_options) {
        m_options = other.m_options;
    }
}



struct PointerGrab::Grab {
    weston_pointer_grab base;
    PointerGrab *parent;
};

struct Seat::Listener {
    wl_listener listener;
    wl_listener capsListener;
    wl_listener selectionListener;
    Seat *seat;
};

static void seatDestroyed(wl_listener *listener, void *data)
{
    delete reinterpret_cast<Seat::Listener *>(listener)->seat;
}

Seat::Seat(Compositor *c, weston_seat *s)
    : m_compositor(c)
    , m_seat(s)
    , m_listener(new Listener)
    , m_pointer(s->pointer ? new Pointer(this, s->pointer) : nullptr)
    , m_keyboard(nullptr)
    , m_popupGrab(nullptr)
    , m_activeScope(nullptr)
{
    m_listener->seat = this;
    m_listener->listener.notify = seatDestroyed;
    wl_signal_add(&s->destroy_signal, &m_listener->listener);
    m_listener->capsListener.notify = [](wl_listener *listener, void *) {
        container_of(listener, Listener, capsListener)->seat->capsUpdated();
    };
    wl_signal_add(&s->updated_caps_signal, &m_listener->capsListener);
    m_listener->selectionListener.notify = [](wl_listener *l, void *) {
        Seat *s = container_of(l, Listener, selectionListener)->seat;
        emit s->selection(s);
    };
    wl_signal_add(&s->selection_signal, &m_listener->selectionListener);
}

Seat::~Seat()
{
    delete m_listener;
    if (m_activeScope) {
        m_activeScope->deactivated(this);
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

Keyboard *Seat::keyboard() const
{
    if (m_keyboard) {
        return m_keyboard;
    }

    if (m_seat->keyboard) {
        Seat *that = const_cast<Seat *>(this);
        that->m_keyboard = new Keyboard(that, m_seat->keyboard);
    }
    return m_keyboard;
}

void Seat::activate(FocusScope *scope)
{
    if (m_activeScope == scope) {
        return;
    }

    if (m_activeScope) {
        m_activeScope->deactivated(this);
    }
    m_activeScope = scope;
    if (m_activeScope) {
        m_activeScope->activated(this);
        Surface *surface = m_activeScope->activeSurface();
        weston_surface_activate(surface ? surface->surface() : nullptr, m_seat);
    }
}

void Seat::sendSelection(wl_client *client)
{
    weston_seat_send_selection(m_seat, client);
}

void Seat::setKeymap(const Keymap &keymap)
{
    Keymap km = keymap;
    km.fill(m_compositor->defaultKeymap());

    xkb_rule_names names = { nullptr, nullptr,
                             km.layout() ? strdup(qPrintable(km.layout().value())) : nullptr,
                             nullptr,
                             km.options() ? strdup(qPrintable(km.options().value())) : nullptr };

    xkb_keymap *xkb = xkb_keymap_new_from_names(m_seat->compositor->xkb_context, &names, (xkb_keymap_compile_flags)0);
    if (xkb) {
        weston_seat_update_keymap(m_seat, xkb);
    }
    free((char *)names.layout);
    free((char *)names.options);
    free((char *)names.rules),
    free((char *)names.model);
    free((char *)names.variant);
}

void Seat::capsUpdated()
{
    // If we now have a keyboard we want to set the focused surface
    if (m_activeScope && m_seat->keyboard) {
        Surface *surface = m_activeScope->activeSurface();
        weston_surface_activate(surface ? surface->surface() : nullptr, m_seat);
    }
}

class Seat::PopupGrab : public PointerGrab
{
public:
    PopupGrab(Seat *s, wl_client *c)
        : seat(s)
        , client(c)
        , initialUp(false)
    {
        start(s);

        /* We must make sure here that this popup was opened after
         * a mouse press, and not just by moving around with other
         * popups already open. */
        if (pointer()->buttonCount() == 0) {
            initialUp = true;
        }
    }
    void focus() override
    {
        double sx, sy;
        View *view = pointer()->pickView(&sx, &sy);

        if (view && view->client() == client) {
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
            return;
        }

        if (state == Pointer::ButtonState::Released) {
            initialUp = true;
        }
    }
    void ended() override
    {
        foreach (ShellSurface *shsurf, surfaces) {
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
        m_popupGrab = new PopupGrab(this, surf->surface()->client());
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

wl_resource *Seat::resource(wl_client *client) const
{
    wl_resource *res;
    wl_resource_for_each(res, &m_seat->base_resource_list) {
        if (wl_resource_get_client(res) == client) {
            return res;
        }
    }
    return nullptr;
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

struct Pointer::Listener {
    wl_listener focusListener;
    Pointer *pointer;
};

Pointer::Pointer(Seat *seat, weston_pointer *p)
       : m_seat(seat)
       , m_pointer(p)
       , m_focus(nullptr)
       , m_currentOutput(nullptr)
       , m_listener(new Listener)
{
    m_hotSpotState.lastTime = 0;
    m_hotSpotState.enterHotZone = 0;

    QObject::connect(m_seat->compositor(), &Compositor::outputRemoved, [this](Output *o) {
        m_defaultGrab.outputs.remove(o);
    });

    m_listener->pointer = this;
    m_listener->focusListener.notify = [](wl_listener *listener, void *) {
        reinterpret_cast<Listener *>(listener)->pointer->updateFocus();
    };
    wl_signal_add(&p->focus_signal, &m_listener->focusListener);
}

Pointer::~Pointer()
{
    wl_list_remove(&m_listener->focusListener.link);
    delete m_listener;
}

View *Pointer::pickView(double *vx, double *vy, const std::function<bool (View *view)> &filter) const
{
    weston_view *view;
    wl_list_for_each(view, &m_seat->compositor()->m_compositor->view_list, link) {
        View *v = View::fromView(view);
        if (filter && !filter(v)) {
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

View *Pointer::pickActivableView(double *vx, double *vy) const
{
    int ix = wl_fixed_to_int(m_pointer->x);
    int iy = wl_fixed_to_int(m_pointer->y);

    weston_view *view;
    wl_list_for_each(view, &m_seat->compositor()->m_compositor->view_list, link) {
        View *v = View::fromView(view);
        Layer *l = v->layer();
        if (l && !l->acceptInput()) {
            continue;
        }

        if (pixman_region32_contains_point(&v->m_view->transform.boundingbox, ix, iy, NULL)) {
            wl_fixed_t fvx, fvy;
            weston_view_from_global_fixed(v->m_view, m_pointer->x, m_pointer->y, &fvx, &fvy);
            if (pixman_region32_contains_point(&v->m_view->surface->input, wl_fixed_to_int(fvx), wl_fixed_to_int(fvy), NULL)) {
                if (vx) *vx = wl_fixed_to_double(fvx);
                if (vy) *vy = wl_fixed_to_double(fvy);
                return v;
            }
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

void Pointer::updateFocus()
{
    View *focus = nullptr;
    weston_view *view = m_pointer->focus;
    if (view) {
        focus = View::fromView(view);
    }

    if (focus != m_focus) {
        if (m_focus) {
            emit m_focus->surface()->pointerFocusLeave(this, m_focus);
        }
        m_focus = focus;
        if (m_focus) {
            emit m_focus->surface()->pointerFocusEnter(this, m_focus);
        }
    }
}

void Pointer::move(double x, double y)
{
    double oldX = this->x();
    double oldY = this->y();
    m_currentOutput = nullptr;
    foreach (Output *o, m_seat->compositor()->outputs()) {
        if (!m_currentOutput || o->contains(oldX, oldY)) {
            m_currentOutput = o;
        }
    }

    if (m_currentOutput) {
        QRect geom = m_currentOutput->geometry();
        const int m = 10;
        const int l = geom.x();
        const int r = geom.right();
        const int t = geom.y();
        const int b = geom.bottom();
        if (x < l + m && y < t + m) {
            if (x - oldX < 0 && x < l)
                x = l;
            if (y - oldY < 0 && y < t)
                y = t;
        } else if (x > r - m && y < t + m) {
            if (x - oldX > 0 && x > r)
                x = r;
            if (y - oldY < 0 && y < t)
                y = t;
        } else if (x > r - m && y > b - m) {
            if (x - oldX > 0 && x > r)
                x = r;
            if (y - oldY > 0 && y > b)
                y = b;
        } else if (x < l + m && y > b - m) {
            if (x - oldX < 0 && x < l)
                x = l;
            if (y - oldY > 0 && y > b)
                y = b;
        }
    }

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
    View *view = pickView(&dx, &dy, [](View *view) {
        Layer *l = view->layer();
        if (l && !l->acceptInput()) {
            return false;
        }
        return true;
    });
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

    if (!m_currentOutput) {
        return;
    }

    const int pushTime = 150;
    PointerHotSpot hs;
    bool inHs = true;
    QRect geom = m_currentOutput->geometry();
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



Keyboard::Keyboard(Seat *seat, weston_keyboard *k)
        : m_seat(seat)
        , m_keyboard(k)
{
}

uint32_t Keyboard::grabSerial() const
{
    return m_keyboard->grab_serial;
}

}
