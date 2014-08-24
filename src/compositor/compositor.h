
#ifndef ORBITAL_COMPOSITOR_H
#define ORBITAL_COMPOSITOR_H

#include <QObject>
#include <QTimer>

struct wl_display;
struct wl_event_loop;
struct wl_client;
struct weston_compositor;
struct weston_surface;

class QProcess;

namespace Orbital {

class Backend;
class Shell;
class Layer;
class Output;
class DummySurface;
class View;
class ChildProcess;
struct Listener;

class Compositor : public QObject
{
    Q_OBJECT
public:
    explicit Compositor(Backend *backend);
    ~Compositor();

    bool init(const QString &socket);
    void quit();

    Layer *rootLayer() const;
    Layer *overlayLayer() const;
    Layer *panelsLayer() const;
    Layer *appsLayer() const;
    Layer *backgroundLayer() const;
    QList<Output *> outputs() const;

    DummySurface *createDummySurface(int width, int height);
    View *pickView(double x, double y, double *vx = nullptr, double *vy = nullptr) const;
    ChildProcess *launchProcess(const QString &path);

    static Compositor *fromCompositor(weston_compositor *c);

private:
    void processEvents();

    wl_display *m_display;
    wl_event_loop *m_loop;
    weston_compositor *m_compositor;
    Listener *m_listener;
    Backend *m_backend;
    Shell *m_shell;
    Layer *m_rootLayer;
    Layer *m_overlayLayer;
    Layer *m_panelsLayer;
    Layer *m_appsLayer;
    Layer *m_backgroundLayer;
    QList<Output *> m_outputs;
    QTimer m_timer;

    friend class Global;
};

class ChildProcess : public QObject
{
    Q_OBJECT
public:

    wl_client *client() const;

private:
    ChildProcess(QProcess *proc, wl_client *client);

    QProcess *m_process;
    wl_client *m_client;

    friend Compositor;

};

}

#endif
