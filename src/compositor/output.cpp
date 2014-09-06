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

#include <weston/compositor.h>

#include "output.h"
#include "workspace.h"
#include "view.h"
#include "layer.h"
#include "compositor.h"
#include "dummysurface.h"
#include "shell.h"
#include "pager.h"

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
      , m_transformRoot(m_compositor->createDummySurface(0, 0))
      , m_background(nullptr)
      , m_currentWs(nullptr)
      , m_backgroundSurface(nullptr)
{
    m_transformRoot->setPos(out->x, out->y);

    m_panelsLayer->append(m_compositor->panelsLayer());

    m_listener->listener.notify = outputDestroyed;
    m_listener->output = this;
    wl_signal_add(&out->destroy_signal, &m_listener->listener);

    connect(this, &Output::moved, this, &Output::onMoved);
}

Output::~Output()
{
    if (m_backgroundSurface) {
        m_backgroundSurface->configure_private = nullptr;
    }

    wl_list_remove(&m_listener->listener.link);
    delete m_listener;
    delete m_panelsLayer;
    delete m_transformRoot;
}

Workspace *Output::currentWorkspace() const
{
    return m_currentWs;
}

class Surface {
public:
    // TODO: delete this
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
    if (m_backgroundSurface) {
        m_backgroundSurface->configure_private = nullptr;
    }
    m_backgroundSurface = surface;
    surface->configure_private = this;
    surface->configure = [](weston_surface *s, int32_t sx, int32_t sy) {
        if (s->configure_private) {
            Output *o = static_cast<Output *>(s->configure_private);
            weston_output_schedule_repaint(o->m_output);
        }
    };


    for (Workspace *ws: m_compositor->shell()->workspaces()) {
        WorkspaceView *wsv = ws->viewForOutput(this);
        wsv->setBackground(surface);
    }
}

void Output::setPanel(weston_surface *surface, int pos)
{
    Surface *s = new Surface(surface, this);
    m_panelsLayer->addView(s->view);
    s->view->setTransformParent(m_transformRoot);
    m_panels << s->view;
}

void Output::setOverlay(weston_surface *surface)
{
    pixman_region32_fini(&surface->pending.input);
    pixman_region32_init_rect(&surface->pending.input, 0, 0, 0, 0);
    Surface *s = new Surface(surface, this);
    m_compositor->overlayLayer()->addView(s->view);
    s->view->setTransformParent(m_transformRoot);
    m_overlays << s->view;
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
        weston_surface *surface = view->surface();
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

Output *Output::fromOutput(weston_output *o)
{
    wl_listener *listener = wl_signal_get(&o->destroy_signal, outputDestroyed);
    if (!listener) {
        return new Output(o);
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
    m_transformRoot->setPos(x(), y());

    for (View *view: m_panels) {
        view->setPos(0, 0);
    }
    for (View *view: m_overlays) {
        view->setPos(0, 0);
    }
    m_compositor->shell()->pager()->updateWorkspacesPosition(this);
}

}
