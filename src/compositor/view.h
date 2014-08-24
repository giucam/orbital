
#ifndef ORBITAL_VIEW_H
#define ORBITAL_VIEW_H

#include <QRectF>

struct wl_client;
struct weston_view;

namespace Orbital
{

class Output;
class Layer;
class WorkspaceView;
class Pointer;
struct Listener;

class View
{
public:
    explicit View(weston_view *view);
    virtual ~View();

    bool isMapped() const;
    double x() const;
    double y() const;
    QRectF geometry() const;
    void setOutput(Output *o);
    void setPos(double x, double y);
    void setPos(const QPointF &p) { setPos(p.x(), p.y()); }
    void setTransformParent(View *p);

    void update();

    Output *output() const;
    wl_client *client() const;
    weston_surface *surface() const;

    static View *fromView(weston_view *v);

private:
    weston_view *m_view;
    Listener *m_listener;
    Output *m_output;

    friend Layer;
    friend Pointer;
};

}

#endif
