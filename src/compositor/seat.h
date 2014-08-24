
#ifndef ORBITAL_SEAT_H
#define ORBITAL_SEAT_H

struct wl_resource;
struct weston_seat;
struct weston_pointer;

namespace Orbital {

struct Listener;
struct Grab;
class Compositor;
class PointerGrab;
class Pointer;
class View;

class Seat
{
public:
    explicit Seat(Compositor *c, weston_seat *seat);
    ~Seat();

    Compositor *compositor() const;
    Pointer *pointer() const;

    static Seat *fromResource(wl_resource *res);

private:
    Compositor *m_compositor;
    weston_seat *m_seat;
    Listener *m_listener;
    Pointer *m_pointer;
};

class Pointer
{
public:
    enum class ButtonState {
        Released = 0,
        Pressed = 1
    };
    enum class Button {
        Left,
        Right,
        Middle
    };

    explicit Pointer(Seat *seat, weston_pointer *pointer);

    View *pickView(double *x = nullptr, double *y = nullptr) const;

    void move(double x, double y);
    int buttonCount() const;
    double x() const;
    double y() const;
    uint32_t grabSerial() const;

private:
    Seat *m_seat;
    weston_pointer *m_pointer;

    friend PointerGrab;
};

class PointerGrab
{
public:
    PointerGrab();
    virtual ~PointerGrab();

    void start(Seat *seat);
//     void start(weston_seat *seat, Cursor c);
    void end();

    Pointer *pointer() const;

protected:
    virtual void focus() {}
    virtual void motion(uint32_t time, double x, double y) {}
    virtual void button(uint32_t time, Pointer::Button button, Pointer::ButtonState state) {}
    virtual void cancel() {}
//     void setCursor(Cursor cursor);
//     void unsetCursor();

private:

    Seat *m_seat;
    Grab *m_grab;

//
//     static const weston_pointer_grab_interface s_shellGrabInterface;

//     friend class Shell;
};

}

#endif
