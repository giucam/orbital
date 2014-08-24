#include <QDebug>

#include <weston/compositor.h>

#include "view.h"
#include "output.h"
#include "workspace.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    View *view;
};

static void viewDestroyed(wl_listener *listener, void *data)
{
}

View::View(weston_view *view)
    : m_view(view)
    , m_listener(new Listener)
    , m_output(nullptr)
{
    m_listener->listener.notify = viewDestroyed;
    m_listener->view = this;
    wl_signal_add(&view->destroy_signal, &m_listener->listener);
}

View::~View()
{
    weston_view_destroy(m_view);
}

bool View::isMapped() const
{
    return weston_view_is_mapped(m_view);
}

double View::x() const
{
    return m_view->geometry.x;
}

double View::y() const
{
    return m_view->geometry.y;
}

void View::setOutput(Output *o)
{
    m_output = o;
    m_view->output = o->m_output;
}

void View::setPos(int x, int y)
{
    weston_view_set_position(m_view, x, y);
    weston_view_geometry_dirty(m_view);
}

void View::setTransformParent(View *p)
{
    weston_view_set_transform_parent(m_view, p->m_view);
    weston_view_update_transform(m_view);
}

void View::update()
{
    weston_view_update_transform(m_view);
}

wl_client *View::client() const
{
    return m_view->surface->resource ? wl_resource_get_client(m_view->surface->resource) : nullptr;
}

Output *View::output() const
{
    return m_output;
}

View *View::fromView(weston_view *v)
{
    wl_listener *listener = wl_signal_get(&v->destroy_signal, viewDestroyed);
    Q_ASSERT(listener);
    return reinterpret_cast<Listener *>(listener)->view;
}

}
