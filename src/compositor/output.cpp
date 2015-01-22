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

#include <QDebug>

#include <compositor.h>

#include "output.h"
#include "workspace.h"
#include "view.h"
#include "layer.h"
#include "compositor.h"
#include "dummysurface.h"
#include "shell.h"
#include "pager.h"
#include "surface.h"

namespace Orbital {

struct Listener {
    wl_listener listener;
    wl_listener frameListener;
    Output *output;
};

static void outputDestroyed(wl_listener *listener, void *data)
{
    delete reinterpret_cast<Listener *>(listener)->output;
}

class Root : public DummySurface
{
public:
    Root(Compositor *c, int w, int h) : DummySurface(c, w, h), view(new View(this)) { }
    View *view;
};

class LockSurface : public DummySurface
{
public:
    LockSurface(Compositor *c, int w, int h) : DummySurface(c, w, h), view(new View(this)) {}
    View *view;
};

Output::Output(weston_output *out)
      : QObject()
      , m_compositor(Compositor::fromCompositor(out->compositor))
      , m_output(out)
      , m_listener(new Listener)
      , m_panelsLayer(new Layer)
      , m_lockLayer(new Layer(m_compositor->rootLayer()))
      , m_transformRoot(new Root(m_compositor, out->width, out->height))
      , m_background(nullptr)
      , m_currentWs(nullptr)
      , m_backgroundSurface(nullptr)
      , m_lockBackgroundSurface(new LockSurface(m_compositor, out->width, out->height))
      , m_lockSurfaceView(nullptr)
      , m_locked(false)
{
    weston_output_init_zoom(m_output);
    m_transformRoot->view->setPos(out->x, out->y);
    m_compositor->baseBackgroundLayer()->addView(m_transformRoot->view);
    m_lockLayer->addView(m_lockBackgroundSurface->view);
    m_lockBackgroundSurface->view->setTransformParent(m_transformRoot->view);
    m_lockLayer->setMask(0, 0, 0, 0);

    m_panelsLayer->append(m_compositor->panelsLayer());

    m_listener->output = this;
    m_listener->listener.notify = outputDestroyed;
    wl_signal_add(&out->destroy_signal, &m_listener->listener);
    m_listener->frameListener.notify = [](wl_listener *l, void *data) {
        Output *o = container_of(l, Listener, frameListener)->output;
        while (!o->m_callbacks.isEmpty()) {
            o->m_callbacks.takeFirst()();
        }
    };
    wl_signal_add(&out->frame_signal, &m_listener->frameListener);

    connect(this, &Output::moved, this, &Output::onMoved);

    if (m_compositor->shell() && m_compositor->shell()->isLocked()) {
        lock(nullptr);
    }
}

Output::~Output()
{
    if (m_backgroundSurface) {
        m_backgroundSurface->setRole(m_backgroundSurface->role(), nullptr);
    }

    qDeleteAll(m_panels);
    qDeleteAll(m_overlays);
    delete m_lockSurfaceView;

    wl_list_remove(&m_listener->listener.link);
    delete m_listener;
    delete m_panelsLayer;
    delete m_transformRoot;
}

Workspace *Output::currentWorkspace() const
{
    return m_currentWs;
}

class OutputSurface
{
public:
    // TODO: delete this
    OutputSurface(Surface *s, Output *o, Surface::Role *role)
        : surface(s)
        , view(new View(s))
    {
        s->setRole(role, [this](int sx, int sy) {
            view->update();
        });
        s->setActivable(false);
        view->setOutput(o);
    }
    ~OutputSurface()
    {
        surface->setRole(surface->role(), nullptr);
    }

    Surface *surface;
    View *view;
};

void Output::setBackground(Surface *surface)
{
    static Surface::Role role;

    if (m_backgroundSurface) {
        m_backgroundSurface->setRole(m_backgroundSurface->role(), nullptr);
    }
    m_backgroundSurface = surface;
    surface->setRole(&role, [this](int sx, int sy) {
        weston_output_schedule_repaint(m_output);
    });
    surface->setActivable(false);

    for (Workspace *ws: m_compositor->shell()->workspaces()) {
        WorkspaceView *wsv = ws->viewForOutput(this);
        wsv->setBackground(surface);
    }
}

void Output::setPanel(Surface *surface, int pos)
{
    class PanelSurface
    {
    public:
        // TODO: delete this
        PanelSurface(Surface *s, Output *o)
            : surface(s)
            , view(new View(s))
            , notified(false)
        {
            static Surface::Role role;
            s->setRole(&role, [this, o](int sx, int sy) {
                view->update();
                if (!notified) {
                    emit o->availableGeometryChanged();
                    notified = true;
                }
            });
            s->setActivable(false);
            view->setOutput(o);
        }
        ~PanelSurface()
        {
            surface->setRole(surface->role(), nullptr);
        }

        Surface *surface;
        View *view;
        bool notified;
    };

    PanelSurface *s = new PanelSurface(surface, this);
    m_panelsLayer->addView(s->view);
    s->view->setTransformParent(m_transformRoot->view);
    connect(s->view, &QObject::destroyed, [this, s]() { m_panels.removeOne(s->view); delete s; });
    m_panels << s->view;
}

void Output::setOverlay(Surface *surface)
{
    static Surface::Role role;
    pixman_region32_fini(&surface->surface()->pending.input);
    pixman_region32_init_rect(&surface->surface()->pending.input, 0, 0, 0, 0);
    OutputSurface *s = new OutputSurface(surface, this, &role);
    m_compositor->overlayLayer()->addView(s->view);
    s->view->setTransformParent(m_transformRoot->view);
    connect(s->view, &QObject::destroyed, [this, s]() { m_overlays.removeOne(s->view); delete s; });
    m_overlays << s->view;
}

void Output::setLockSurface(Surface *surface)
{
    delete m_lockSurfaceView;

    static Surface::Role role;
    OutputSurface *s = new OutputSurface(surface, this, &role);
    surface->setActivable(true);
    m_lockLayer->addView(s->view);
    m_lockSurfaceView = s->view;
    s->view->setTransformParent(m_transformRoot->view);
    connect(s->view, &QObject::destroyed, [this, s]() { m_lockSurfaceView = nullptr; delete s; });
}

Surface *Output::lockSurface() const
{
    return m_lockSurfaceView ? m_lockSurfaceView->surface() : nullptr;
}

void Output::lock(const std::function<void ()> &done)
{
    m_locked = true;
    m_lockLayer->setMask(x(), y(), width(), height());
    repaint(done);
}

void Output::unlock()
{
    m_locked = false;
    m_lockLayer->setMask(0, 0, 0, 0);
    if (m_lockSurfaceView) {
        emit m_lockSurfaceView->surface()->unmapped();
    }
    repaint();
}

void Output::repaint(const std::function<void ()> &done)
{
    weston_output_schedule_repaint(m_output);
    if (done) {
        m_callbacks << done;
    }
}

void Output::setPos(int x, int y)
{
    weston_output_move(m_output, x, y);
}

int Output::id() const
{
    return m_output->id;
}

int Output::x() const
{
    return m_output->x;
}

int Output::y() const
{
    return m_output->y;
}

int Output::width() const
{
    return m_output->width;
}

int Output::height() const
{
    return m_output->height;
}

QRect Output::geometry() const
{
    return QRect(m_output->x, m_output->y, m_output->width, m_output->height);
}

QRect Output::availableGeometry() const
{
    pixman_region32_t area;
    pixman_region32_init_rect(&area, 0, 0, m_output->width, m_output->height);

    for (View *view: m_panels) {
        weston_surface *surface = view->surface()->surface();
        pixman_region32_t surf;
        pixman_region32_init(&surf);
        pixman_region32_copy(&surf, &surface->input);
        pixman_region32_translate(&surf, view->x(), view->y());
        pixman_region32_subtract(&area, &area, &surf);
        pixman_region32_fini(&surf);
    }
    pixman_box32_t *box = pixman_region32_extents(&area);
    pixman_region32_fini(&area);
    return QRect(box->x1, box->y1, box->x2 - box->x1, box->y2 - box->y1);
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

View *Output::rootView() const
{
    return m_transformRoot->view;
}

QString Output::name() const
{
    return QString(m_output->name);
}

bool Output::contains(double x, double y) const
{
    return geometry().contains(x, y);
}

Output *Output::fromOutput(weston_output *o)
{
    wl_listener *listener = wl_signal_get(&o->destroy_signal, outputDestroyed);
    if (!listener) {
        return nullptr;
    }

    return reinterpret_cast<Listener *>(listener)->output;
}

Output *Output::fromResource(wl_resource *res)
{
    weston_output *o = static_cast<weston_output *>(wl_resource_get_user_data(res));
    return fromOutput(o);
}

void Output::onMoved()
{
    // when an output is removed weston rearranges the remaimining ones and the views, so be sure
    // to move them back where they should be
    m_transformRoot->view->setPos(x(), y());
    if (m_locked) {
        m_lockLayer->setMask(x(), y(), width(), height());
    }

    for (View *view: m_panels) {
        view->setPos(0, 0);
    }
    for (View *view: m_overlays) {
        view->setPos(0, 0);
    }
    if (Shell *shell = m_compositor->shell()) {
        shell->pager()->updateWorkspacesPosition(this);
    }
}

}
