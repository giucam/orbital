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

#ifndef ORBITAL_COMPOSITOR_H
#define ORBITAL_COMPOSITOR_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QMultiHash>
#include <QVector>

#include "global.h"

struct wl_display;
struct wl_event_loop;
struct wl_client;
struct weston_compositor;
struct weston_surface;
struct weston_output;

class QProcess;
class QObjectCleanupHandler;
class QSocketNotifier;

namespace Orbital {

class Backend;
class Shell;
class Layer;
class Output;
class DummySurface;
class View;
class ChildProcess;
class Seat;
class ButtonBinding;
class KeyBinding;
class AxisBinding;
class HotSpotBinding;
class Surface;
struct Listener;
enum class PointerButton : unsigned char;
enum class PointerAxis : unsigned char;
enum class KeyboardModifiers : unsigned char;
enum class PointerHotSpot : unsigned char;

class Compositor : public QObject
{
    Q_OBJECT
public:
    enum class Layer {
        Lock,
        Overlay,
        Fullscreen,
        Panels,
        Sticky,
        Dashboard,
        Apps,
        Background,
        BaseBackground,
        Minimized,
    };

    explicit Compositor(Backend *backend);
    ~Compositor();

    bool init(const QString &socket);
    void quit();

    inline wl_display *display() const { return m_display; }
    Shell *shell() const;
    Orbital::Layer *layer(Layer l) const;
    QList<Output *> outputs() const;
    QList<Seat *> seats() const;
    const Keymap &defaultKeymap() const { return m_defaultKeymap; }

    uint32_t nextSerial() const;

    View *pickView(double x, double y, double *vx = nullptr, double *vy = nullptr) const;
    ChildProcess *launchProcess(const QString &path);

    void kill(Surface *surface);

    ButtonBinding *createButtonBinding(PointerButton button, KeyboardModifiers modifiers);
    KeyBinding *createKeyBinding(uint32_t key, KeyboardModifiers modifiers);
    AxisBinding *createAxisBinding(PointerAxis axis, KeyboardModifiers modifiers);
    HotSpotBinding *createHotSpotBinding(PointerHotSpot hs);

    void handleHotSpot(Seat *seat, uint32_t time, PointerHotSpot hs);

    static Compositor *fromCompositor(weston_compositor *c);

signals:
    void outputCreated(Output *output);
    void outputRemoved(Output *output);
    void sessionActivated(bool active);
    void seatCreated(Seat *seat);

private:
    void processEvents();
    void processIdle();
    void outputDestroyed();
    void handleSignal();
    void newOutput(weston_output *o);
    void fakeRepaint();

    wl_display *m_display;
    wl_event_loop *m_loop;
    weston_compositor *m_compositor;
    Listener *m_listener;
    Backend *m_backend;
    Shell *m_shell;
    QVector<Orbital::Layer *> m_layers;
    QList<Output *> m_outputs;
    QTimer m_fakeRepaintLoopTimer;
    QObjectCleanupHandler *m_bindingsCleanupHandler;
    QSocketNotifier *m_signalsNotifier;
    QJsonObject m_config;
    QMultiHash<int, HotSpotBinding *> m_hotSpotBindings;
    Keymap m_defaultKeymap;

    friend class Global;
    friend class XWayland;
    friend class Pointer;
    friend DummySurface;
};

class ChildProcess : public QObject
{
    Q_OBJECT
public:
    ~ChildProcess();

    void restart();
    void setAutoRestart(bool enabled);
    wl_client *client() const;

signals:
    void givingUp();

private:
    struct Listener;

    ChildProcess(wl_display *display, const QString &program);
    void start();
    void finished();

    wl_display *m_display;
    QString m_program;
    wl_client *m_client;
    bool m_autoRestart;
    Listener *m_listener;
    int m_startTime;
    int m_deathCount;

    friend Compositor;

};

}

#endif
