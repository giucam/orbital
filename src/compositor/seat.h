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

#include <QObject>
#include <QPointF>
#include <QLinkedList>
#include <QSet>

struct wl_resource;
struct weston_seat;
struct weston_pointer;

namespace Orbital {

struct Listener;
class Compositor;
class PointerGrab;
class Pointer;
class Keyboard;
class View;
class Surface;
class ShellSurface;
class Workspace;
class Output;
class FocusScope;
enum class PointerButton : unsigned char;

class Seat : public QObject
{
    Q_OBJECT
public:
    explicit Seat(Compositor *c, weston_seat *seat);
    ~Seat();

    Compositor *compositor() const;
    Pointer *pointer() const;
    Keyboard *keyboard() const;

    void activate(FocusScope *scope);

    void grabPopup(ShellSurface *surf);
    void ungrabPopup(ShellSurface *surf);

    static Seat *fromSeat(weston_seat *seat);
    static Seat *fromResource(wl_resource *res);

signals:
    void pointerMotion(Pointer *pointer);

private:
    void capsUpdated();
    class PopupGrab;

    Compositor *m_compositor;
    weston_seat *m_seat;
    Listener *m_listener;
    Pointer *m_pointer;
    Keyboard *m_keyboard;
    PopupGrab *m_popupGrab;
    FocusScope *m_activeScope;

    friend FocusScope;
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
    Busy = 11,
    Kill = 12
};

enum class PointerHotSpot : unsigned char {
    TopLeftCorner,
    TopRightCorner,
    BottomLeftCorner,
    BottomRightCorner
};

class Pointer
{
public:
    enum class ButtonState {
        Released = 0,
        Pressed = 1
    };

    explicit Pointer(Seat *seat, weston_pointer *pointer);
    ~Pointer();

    inline Seat *seat() const { return m_seat; }
    View *pickView(double *x = nullptr, double *y = nullptr) const;

    void setFocus(View *view);
    void setFocus(View *view, double x, double y);
    inline void setFocus(View *view, const QPointF &p) { setFocus(view, p.x(), p.y()); }
    View *focus() const;
    void move(double x, double y);
    void sendMotion(uint32_t time);
    void sendButton(uint32_t time, PointerButton button, ButtonState state);
    int buttonCount() const;
    double x() const;
    double y() const;

    Output *currentOutput() const;

    bool isGrabActive() const;
    PointerGrab *activeGrab() const;
    uint32_t grabSerial() const;
    uint32_t grabTime() const;
    PointerButton grabButton() const;
    QPointF grabPos() const;

    void defaultGrabFocus();
    void defaultGrabMotion(uint32_t time, double x, double y);
    void defaultGrabButton(uint32_t time, uint32_t btn, uint32_t state);

private:
    void setFocusFixed(View *view, wl_fixed_t x, wl_fixed_t y);
    void handleMotionBinding(uint32_t time, double x, double y);

    Seat *m_seat;
    weston_pointer *m_pointer;
    Output *m_currentOutput;
    struct {
        QSet<Output *> outputs;
    } m_defaultGrab;
    struct {
        uint32_t lastTime;
        uint32_t enterHotZone;
    } m_hotSpotState;

    friend PointerGrab;
    friend Seat;
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
    virtual void button(uint32_t time, PointerButton button, Pointer::ButtonState state) {}
    virtual void cancel() {}
    virtual void ended() {}
//     void setCursor(Cursor cursor);
//     void unsetCursor();

private:
    struct Grab;

    static PointerGrab *fromGrab(weston_pointer_grab *grab);

    Seat *m_seat;
    Grab *m_grab;

//
//     static const weston_pointer_grab_interface s_shellGrabInterface;

    friend Pointer;
};

class Keyboard
{
public:
    uint32_t grabSerial() const;

private:
    explicit Keyboard(Seat *seat, weston_keyboard *k);

    Seat *m_seat;
    weston_keyboard *m_keyboard;

    friend Seat;
};

}

#endif
