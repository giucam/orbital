
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
    m_transformRoot->setPos(out->x, out->y);

    m_panelsLayer->append(m_compositor->panelsLayer());
    m_backgroundLayer->append(m_compositor->backgroundLayer());

    m_listener->listener.notify = outputDestroyed;
    m_listener->output = this;
    wl_signal_add(&out->destroy_signal, &m_listener->listener);
}

void Output::viewWorkspace(Workspace *ws)
{
    if (m_currentWsv) {
        m_currentWsv->detach();
    }
    WorkspaceView *view = ws->viewForOutput(this);
    view->attach(m_transformRoot, m_output->x, m_output->y);
    m_currentWsv = view;

    weston_output_schedule_repaint(m_output);
}

class Surface {
public:
    Surface(weston_surface *s, Output *o)
    {
        s->configure_private = this;
        s->configure = [](weston_surface *s, int32_t sx, int32_t sy) {
            Surface *o = static_cast<Surface *>(s->configure_private);
            // TODO: Find out if and how to remove this
            o->view->update();
        };

        weston_view *v = weston_view_create(s);
        view = new View(v);
        view->setOutput(o);
    }

    View *view;
};

void Output::setBackground(weston_surface *surface)
{
    Surface *s = new Surface(surface, this);
    m_backgroundLayer->addView(s->view);
    s->view->setTransformParent(m_transformRoot);
}

void Output::setPanel(weston_surface *surface, int pos)
{
    Surface *s = new Surface(surface, this);
    m_panelsLayer->addView(s->view);
    s->view->setTransformParent(m_transformRoot);
}

void Output::setOverlay(weston_surface *surface)
{
    pixman_region32_fini(&surface->pending.input);
    pixman_region32_init_rect(&surface->pending.input, 0, 0, 0, 0);
    Surface *s = new Surface(surface, this);
    m_compositor->overlayLayer()->addView(s->view);
    s->view->setTransformParent(m_transformRoot);
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

wl_resource *Output::resource(wl_client *client) const
{
    wl_resource *resource;
    wl_resource_for_each(resource, &m_output->resource_list) {
        if (wl_resource_get_client(resource) == client) {
            return resource;
        }
    }

    return nullptr;
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
