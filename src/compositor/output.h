
#ifndef ORBITAL_OUTPUT_H
#define ORBITAL_OUTPUT_H

#include <QObject>

struct wl_resource;
struct weston_output;
struct weston_surface;

namespace Orbital {

class Compositor;
class Workspace;
class View;
class Layer;
class WorkspaceView;
class DummySurface;
struct Listener;

class Output : public QObject
{
    Q_OBJECT
public:
    explicit Output(weston_output *out);

    void viewWorkspace(Workspace *ws);
    void setBackground(weston_surface *surface);
    void setPanel(weston_surface *surface, int pos);
    void setOverlay(weston_surface *surface);

    int id() const;
    int width() const;
    int height() const;

    static Output *fromResource(wl_resource *res);

private:
    Compositor *m_compositor;
    weston_output *m_output;
    Listener *m_listener;
    Layer *m_panelsLayer;
    Layer *m_backgroundLayer;
    DummySurface *m_transformRoot;
    View *m_background;
    WorkspaceView *m_currentWsv;

    friend View;
};

}

#endif
