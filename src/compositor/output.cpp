
#include <QDebug>

#include <weston/compositor.h>

#include "output.h"
#include "workspace.h"
#include "view.h"
#include "layer.h"
#include "compositor.h"
#include "dummysurface.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    Output *output;
};

static void outputDestroyed(wl_listener *listener, void *data)
{
    delete reinterpret_cast<Listener *>(listener)->output;
}

Output::Output(weston_output *out)
      : QObject()
      , m_compositor(Compositor::fromCompositor(out->compositor))
      , m_output(out)
      , m_listener(new Listener)
      , m_panelsLayer(new Layer)
      , m_backgroundLayer(new Layer)
      , m_transformRoot(m_compositor->createDummySurface(0, 0))
      , m_background(nullptr)
      , m_currentWsv(nullptr)
{
    m_panelsLayer->append(m_compositor->panelsLayer());
    m_backgroundLayer->append(m_compositor->backgroundLayer());

    m_listener->listener.notify = outputDestroyed;
    m_listener->output = this;
    wl_signal_add(&out->destroy_signal, &m_listener->listener);
}

void Output::viewWorkspace(Workspace *ws)
{
    WorkspaceView *view = ws->viewForOutput(this);
    view->setPos(m_output->x, m_output->y);
    view->addTransformChild(m_transformRoot);
    m_currentWsv = view;
}

void Output::setBackground(weston_surface *surface)
{
    surface->configure_private = this;
    surface->configure = [](weston_surface *s, int32_t sx, int32_t sy) {
        Output *o = static_cast<Output *>(s->configure_private);
        // TODO: Find out if and how to remove this
        o->m_background->update();
    };
    surface->output = m_output;
    weston_view *v = weston_view_create(surface);
    m_background = new View(v);
    m_background->setOutput(this);
    m_backgroundLayer->addView(m_background);
    m_background->setTransformParent(m_transformRoot);
}

void Output::setPanel(weston_surface *surface, int pos)
{
    surface->configure_private = this;
    surface->configure = [](weston_surface *s, int32_t sx, int32_t sy) { };
    surface->output = m_output;
    weston_view *v = weston_view_create(surface);
    View *view = new View(v);
    view->setOutput(this);
    m_panelsLayer->addView(view);
    view->setTransformParent(m_transformRoot);
}

void Output::setOverlay(weston_surface *surface)
{
    surface->configure_private = this;
    surface->configure = [](weston_surface *s, int32_t sx, int32_t sy) { };
    surface->output = m_output;
    weston_view *v = weston_view_create(surface);
    View *view = new View(v);
    view->setOutput(this);
    m_compositor->overlayLayer()->addView(view);
    view->setTransformParent(m_transformRoot);
}

int Output::id() const
{
    return m_output->id;
}

int Output::width() const
{
    return m_output->width;
}

int Output::height() const
{
    return m_output->height;
}

Output *Output::fromResource(wl_resource *res)
{
    weston_output *o = static_cast<weston_output *>(wl_resource_get_user_data(res));
    wl_listener *listener = wl_signal_get(&o->destroy_signal, outputDestroyed);
    if (!listener) {
        return new Output(o);
    }

    return reinterpret_cast<Listener *>(listener)->output;
}

}
