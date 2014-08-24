
#include <linux/input.h>

#include <QDebug>

#include <weston/compositor.h>

#include "seat.h"
#include "compositor.h"

namespace Orbital {

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

Seat *Seat::fromResource(wl_resource *res)
{
    weston_seat *s = static_cast<weston_seat *>(wl_resource_get_user_data(res));
    wl_listener *listener = wl_signal_get(&s->destroy_signal, seatDestroyed);
    if (!listener) {
        return new Seat(Compositor::fromCompositor(s->compositor), s);
    }

    return reinterpret_cast<Listener *>(listener)->seat;
}



// -- Pointer

Pointer::Pointer(Seat *seat, weston_pointer *p)
       : m_seat(seat)
       , m_pointer(p)
{
}

View *Pointer::pickView(double *vx, double *vy) const
{
    return m_seat->compositor()->pickView(x(), y(), vx, vy);
}

void Pointer::move(double x, double y)
{
    weston_pointer_move(m_pointer, wl_fixed_from_double(x), wl_fixed_from_double(y));
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

uint32_t Pointer::grabSerial() const
{
    return m_pointer->grab_serial;
}

// -- PointerGrab

struct Grab {
    weston_pointer_grab base;
    PointerGrab *parent;
};

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
            Pointer::Button btn;
            switch (button) {
                case BTN_LEFT: btn = Pointer::Button::Left; break;
                case BTN_RIGHT: btn = Pointer::Button::Right; break;
                case BTN_MIDDLE: btn = Pointer::Button::Middle; break;
                default:
                    qWarning("Unknown mouse button: %d\n", button);
                    return;
            }
            fromGrab(base)->button(time, btn, (Pointer::ButtonState)state);
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
    m_seat = seat;

    weston_pointer_start_grab(m_seat->pointer()->m_pointer, &m_grab->base);
}

void PointerGrab::end()
{
    if (m_seat) {
        weston_pointer_end_grab(m_seat->pointer()->m_pointer);
        m_seat = nullptr;
    }
}

Pointer *PointerGrab::pointer() const
{
    return m_seat->pointer();
}


}
