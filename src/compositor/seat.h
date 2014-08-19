
#ifndef ORBITAL_SEAT_H
#define ORBITAL_SEAT_H

struct wl_resource;
struct weston_seat;

namespace Orbital {

struct Listener;

class Seat
{
public:
    explicit Seat(weston_seat *seat);
    ~Seat();

    static Seat *fromResource(wl_resource *res);

private:
    weston_seat *m_seat;
    Listener *m_listener;
};

}

#endif
