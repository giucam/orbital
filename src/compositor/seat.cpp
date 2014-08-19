
#include <weston/compositor.h>

#include "seat.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    Seat *seat;
};

static void seatDestroyed(wl_listener *listener, void *data)
{
    delete reinterpret_cast<Listener *>(listener)->seat;
}

Seat::Seat(weston_seat *s)
    : m_seat(s)
    , m_listener(new Listener)
{
    m_listener->listener.notify = seatDestroyed;
    m_listener->seat = this;
    wl_signal_add(&s->destroy_signal, &m_listener->listener);
}

Seat::~Seat()
{
    delete m_listener;
}

Seat *Seat::fromResource(wl_resource *res)
{
    weston_seat *s = static_cast<weston_seat *>(wl_resource_get_user_data(res));
    wl_listener *listener = wl_signal_get(&s->destroy_signal, seatDestroyed);
    if (!listener) {
        return new Seat(s);
    }

    return reinterpret_cast<Listener *>(listener)->seat;
}

}
