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
class ShellSurface;

class Seat
{
public:
    explicit Seat(Compositor *c, weston_seat *seat);
    ~Seat();

    Compositor *compositor() const;
    Pointer *pointer() const;

    void grabPopup(ShellSurface *surf);

    static Seat *fromSeat(weston_seat *seat);
    static Seat *fromResource(wl_resource *res);

private:
    Compositor *m_compositor;
    weston_seat *m_seat;
    Listener *m_listener;
    Pointer *m_pointer;

    PointerGrab *m_popupGrab;
};

enum class PointerCursor: unsigned int {
    None = 0,
    ResizeTop = 1,
    ResizeBottom = 2,
    Arrow = 3,
    ResizeLeft = 4,
    ResizeTopLeft = ResizeTop | ResizeLeft,
    ResizeBottomLeft = ResizeBottom | ResizeLeft,
    Move = 7,
    ResizeRight = 8,
    ResizeTopRight = ResizeTop | ResizeRight,
    ResizeBottomRight = ResizeBottom | ResizeRight,
    Busy = 11
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

    void setFocus(View *view, double x, double y);
    View *focus() const;
    void move(double x, double y);
    void sendMotion(uint32_t time);
    void sendButton(uint32_t time, Button button, ButtonState state);
    int buttonCount() const;
    double x() const;
    double y() const;

    uint32_t grabSerial() const;
    uint32_t grabTime() const;
    QPointF grabPos() const;

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
    void start(Seat *seat, PointerCursor c);
    void end();

    Pointer *pointer() const;
    void setCursor(PointerCursor cursor);

protected:
    virtual void focus() {}
    virtual void motion(uint32_t time, double x, double y) {}
    virtual void button(uint32_t time, Pointer::Button button, Pointer::ButtonState state) {}
    virtual void cancel() {}
    virtual void ended() {}
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
