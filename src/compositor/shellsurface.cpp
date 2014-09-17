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

#include "shellsurface.h"
#include "shell.h"
#include "shellview.h"
#include "workspace.h"
#include "output.h"
#include "compositor.h"
#include "seat.h"
#include "pager.h"

namespace Orbital
{

struct Listener {
    wl_listener listener;
    ShellSurface *surface;
};

void ShellSurface::surfaceDestroyed(wl_listener *listener, void *data)
{
    ShellSurface *surface = reinterpret_cast<Listener *>(listener)->surface;
    delete surface;
}

void ShellSurface::parentSurfaceDestroyed(wl_listener *listener, void *data)
{
    ShellSurface *surface = reinterpret_cast<Listener *>(listener)->surface;
    surface->m_parent = nullptr;
    surface->m_nextType = Type::None;
    wl_list_remove(&surface->m_parentListener->listener.link);
    wl_list_init(&surface->m_parentListener->listener.link);
}

ShellSurface::ShellSurface(Shell *shell, weston_surface *surface)
            : Object(shell)
            , m_shell(shell)
            , m_surface(surface)
            , m_listener(new Listener)
            , m_configureSender(nullptr)
            , m_resizeEdges(Edges::None)
            , m_forceMap(false)
            , m_type(Type::None)
            , m_nextType(Type::None)
            , m_parentListener(new Listener)
            , m_popup({ 0, 0, nullptr })
            , m_toplevel({ false, false })
            , m_transient({ 0, 0, false })
{
    m_listener->listener.notify = surfaceDestroyed;
    m_listener->surface = this;
    wl_signal_add(&surface->destroy_signal, &m_listener->listener);

    m_parentListener->listener.notify = parentSurfaceDestroyed;
    m_parentListener->surface = this;
    wl_list_init(&m_parentListener->listener.link);

    surface->configure_private = this;
    surface->configure = staticConfigure;

    for (Output *o: shell->compositor()->outputs()) {
        ShellView *view = new ShellView(this, weston_view_create(m_surface));
        view->setDesignedOutput(o);
        m_views.insert(o->id(), view);
    }
    connect(shell->compositor(), &Compositor::outputCreated, this, &ShellSurface::outputCreated);
    connect(shell->compositor(), &Compositor::outputRemoved, this, &ShellSurface::outputRemoved);
}

ShellSurface::~ShellSurface()
{
    wl_list_remove(&m_listener->listener.link);
    qDeleteAll(m_views);
}

ShellView *ShellSurface::viewForOutput(Output *o)
{
    return m_views.value(o->id());
}

void ShellSurface::setWorkspace(Workspace *ws)
{
    m_workspace = ws;
}

Compositor *ShellSurface::compositor() const
{
    return m_shell->compositor();
}

Workspace *ShellSurface::workspace() const
{
    return m_workspace;
}

wl_client *ShellSurface::client() const
{
    return m_surface->resource ? wl_resource_get_client(m_surface->resource) : nullptr;
}

weston_surface *ShellSurface::surface() const
{
    return m_surface;
}

bool ShellSurface::isMapped() const
{
    return weston_surface_is_mapped(m_surface);
}

void ShellSurface::setConfigureSender(ConfigureSender sender)
{
    m_configureSender = sender;
}

void ShellSurface::setToplevel()
{
    m_nextType = Type::Toplevel;
    m_toplevel.maximized = false;
    m_toplevel.fullscreen = false;
    wl_list_remove(&m_parentListener->listener.link);
    wl_list_init(&m_parentListener->listener.link);
}

void ShellSurface::setTransient(weston_surface *parent, int x, int y, bool inactive)
{
    m_parent = parent;
    m_transient.x = x;
    m_transient.y = y;
    m_transient.inactive = inactive;

    wl_list_remove(&m_parentListener->listener.link);
    wl_list_init(&m_parentListener->listener.link);
    wl_signal_add(&parent->destroy_signal, &m_parentListener->listener);

    m_nextType = Type::Transient;
}

void ShellSurface::setPopup(weston_surface *parent, Seat *seat, int x, int y)
{
    m_parent = parent;
    m_popup.x = x;
    m_popup.y = y;
    m_popup.seat = seat;

    wl_list_remove(&m_parentListener->listener.link);
    wl_list_init(&m_parentListener->listener.link);
    wl_signal_add(&parent->destroy_signal, &m_parentListener->listener);

    m_nextType = Type::Popup;
}

void ShellSurface::setMaximized()
{
    m_nextType = Type::Toplevel;
    m_toplevel.maximized = true;
    m_toplevel.fullscreen = false;

    Output *output = selectOutput();

    QRect rect = output->availableGeometry();
    qDebug() << "Maximizing surface on output" << output << "with rect" << rect;
    sendConfigure(rect.width(), rect.height());
}

void ShellSurface::setFullscreen()
{
    m_nextType = Type::Toplevel;
    m_toplevel.fullscreen = true;
    m_toplevel.maximized = false;

    Output *output = selectOutput();

    QRect rect = output->availableGeometry();
    qDebug() << "Fullscrening surface on output" << output << "with rect" << rect;
    sendConfigure(rect.width(), rect.height());
}

void ShellSurface::move(Seat *seat)
{

    class MoveGrab : public PointerGrab
    {
    public:
        void motion(uint32_t time, double x, double y) override
        {
            pointer()->move(x, y);

            double moveX = x + dx;
            double moveY = y + dy;

            for (View *view: shsurf->m_views) {
                view->setPos(moveX, moveY);
            }
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            if (pointer()->buttonCount() == 0 && state == Pointer::ButtonState::Released) {
    //             shsurf->moveEndSignal(shsurf);
    //             shsurf->m_runningGrab = nullptr;
                end();
            }
        }
        void ended() override
        {
            delete this;
        }

        ShellSurface *shsurf;
        double dx, dy;
    };

    MoveGrab *move = new MoveGrab;

//     if (m_runningGrab) {
//         return;
//     }
//
//     if (m_type == ShellSurface::Type::TopLevel && m_state.fullscreen) {
//         return;
//     }

//     MoveGrab *move = new MoveGrab;
//     if (!move)
//         return;
//

    View *view = seat->pointer()->pickView();
    move->dx = view->x() - seat->pointer()->x();
    move->dy = view->y() - seat->pointer()->y();
    move->shsurf = this;
//     m_runningGrab = move;

    move->start(seat, PointerCursor::Move);
//     moveStartSignal(this);
}

void ShellSurface::resize(Seat *seat, Edges edges)
{
    class ResizeGrab : public PointerGrab
    {
    public:
        void motion(uint32_t time, double x, double y) override
        {
            pointer()->move(x, y);

            QPointF from = view->mapFromGlobal(pointer()->grabPos());
            QPointF to = view->mapFromGlobal(QPointF(x, y));
            QPointF d = to - from;

            int32_t w = width;
            if (shsurf->m_resizeEdges & ShellSurface::Edges::Left) {
                w -= d.x();
            } else if (shsurf->m_resizeEdges & ShellSurface::Edges::Right) {
                w += d.x();
            }

            int32_t h = height;
            if (shsurf->m_resizeEdges & ShellSurface::Edges::Top) {
                h -= d.y();
            } else if (shsurf->m_resizeEdges & ShellSurface::Edges::Bottom) {
                h += d.y();
            }

            shsurf->sendConfigure(w, h);
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            if (pointer()->buttonCount() == 0 && state == Pointer::ButtonState::Released) {
                end();
            }
        }
        void ended() override
        {
            shsurf->m_resizeEdges = ShellSurface::Edges::None;
            delete this;
        }

        ShellSurface *shsurf;
        View *view;
        int32_t width, height;
    };

//     if (m_runningGrab) {
//         return;
//     }
//
    ResizeGrab *grab = new ResizeGrab;

    int e = (int)edges;
    if (e == 0 || e > 15 || (e & 3) == 3 || (e & 12) == 12) {
        return;
    }

    m_resizeEdges = edges;


    QRect rect = geometry();
    grab->width = m_width = rect.width();
    grab->height = m_height = rect.height();
    grab->shsurf = this;
    grab->view = seat->pointer()->pickView();

    grab->start(seat, (PointerCursor)e);
}

void ShellSurface::unmap()
{
    for (ShellView *v: m_views) {
        v->cleanupAndUnmap();
    }
}

void ShellSurface::sendPopupDone()
{
    m_nextType = Type::None;
    m_popup.seat = nullptr;
    emit popupDone();
}

void ShellSurface::setTitle(const QString &t)
{
    if (m_title != t) {
        m_title = t;
        emit titleChanged();
    }
}

void ShellSurface::setGeometry(int x, int y, int w, int h)
{
    m_nextGeometry = QRect(x, y, w, h);
}

bool ShellSurface::isFullscreen() const
{
    return m_type == Type::Toplevel && m_toplevel.fullscreen;
}

bool ShellSurface::isInactive() const
{
    return m_type == Type::Transient && m_transient.inactive;
}

QRect ShellSurface::geometry() const
{
    if (m_geometry.isValid()) {
        return m_geometry;
    }
    return surfaceTreeBoundingBox();
}

QString ShellSurface::title() const
{
    return m_title;
}

/*
 * Returns the bounding box of a surface and all its sub-surfaces,
 * in the surface coordinates system. */
QRect ShellSurface::surfaceTreeBoundingBox() const
{
    pixman_region32_t region;
    pixman_box32_t *box;
    weston_subsurface *subsurface;

    pixman_region32_init_rect(&region, 0, 0,
                              m_surface->width,
                              m_surface->height);

    wl_list_for_each(subsurface, &m_surface->subsurface_list, parent_link) {
        pixman_region32_union_rect(&region, &region,
                                   subsurface->position.x,
                                   subsurface->position.y,
                                   subsurface->surface->width,
                                   subsurface->surface->height);
    }

    box = pixman_region32_extents(&region);
    QRect rect(box->x1, box->y1, box->x2 - box->x1, box->y2 - box->y1);
    pixman_region32_fini(&region);

    return rect;
}

void ShellSurface::staticConfigure(weston_surface *s, int32_t x, int32_t y)
{
    static_cast<ShellSurface *>(s->configure_private)->configure(x, y);
}

void ShellSurface::configure(int x, int y)
{
    if (m_surface->width == 0 && m_popup.seat) {
        m_popup.seat->ungrabPopup(this);
    }

    if (m_surface->width == 0) {
        return;
    }

    updateState();

    if (m_type == Type::None) {
        return;
    }

    bool wasMapped = isMapped();
    m_shell->configure(this);
    if (!m_workspace) {
        return;
    }

    if (m_type == Type::Toplevel) {
        int dy = 0;
        int dx = 0;
        QRect rect = geometry();
        if ((int)m_resizeEdges) {
            if (m_resizeEdges & Edges::Top) {
                dy = m_height - rect.height();
            }
            if (m_resizeEdges & Edges::Left) {
                dx = m_width - rect.width();
            }
            m_height = rect.height();
            m_width = rect.width();
        }

        bool map = !isMapped() || m_state.maximized != m_toplevel.maximized || m_state.fullscreen != m_toplevel.fullscreen ||
                   m_state.size != rect.size() || m_forceMap;
        m_forceMap = false;
        m_state.size = rect.size();
        m_state.maximized = m_toplevel.maximized;
        m_state.fullscreen = m_toplevel.fullscreen;

        for (ShellView *view: m_views) {
            view->configureToplevel(map, m_toplevel.maximized, m_toplevel.fullscreen, dx, dy);
        }
    } else if (m_type == Type::Popup) {
        ShellSurface *parent = ShellSurface::fromSurface(m_parent);
        if (!parent) {
            qWarning("Trying to map a popup without a ShellSurface parent.");
            return;
        }

        for (Output *o: m_shell->compositor()->outputs()) {
            ShellView *view = viewForOutput(o);
            ShellView *parentView = parent->viewForOutput(o);

            view->configurePopup(parentView, m_popup.x, m_popup.y);
        }
        m_popup.seat->grabPopup(this);
    } else if (m_type == Type::Transient) {
        ShellSurface *parent = ShellSurface::fromSurface(m_parent);
        if (!parent) {
            View *parentView = View::fromView(container_of(m_parent->views.next, weston_view, surface_link));
            ShellView *view = viewForOutput(parentView->output());
            view->configureTransient(parentView, m_transient.x, m_transient.y);
        } else {
            for (Output *o: m_shell->compositor()->outputs()) {
                ShellView *view = viewForOutput(o);
                ShellView *parentView = parent->viewForOutput(o);

                view->configureTransient(parentView, m_transient.x, m_transient.y);
            }
        }
    }
    weston_surface_damage(m_surface);

    if (!wasMapped && isMapped()) {
        emit mapped();
    }
}

void ShellSurface::updateState()
{
    m_type = m_nextType;
    m_geometry = m_nextGeometry;
}

void ShellSurface::sendConfigure(int w, int h)
{
    if (m_configureSender) {
        m_configureSender(m_surface, w, h);
    }
}

Output *ShellSurface::selectOutput()
{
    struct Out {
        Output *output;
        int vote;
    };
    QList<Out> candidates;
    for (Output *o: m_shell->compositor()->outputs()) {
        candidates.append({ o, m_shell->pager()->isWorkspaceActive(m_workspace, o) ? 10 : 0 });
    }

    Output *output = nullptr;
    if (candidates.isEmpty()) {
        return nullptr;
    } else if (candidates.size() == 1) {
        output = candidates.first().output;
    } else {
        QList<Seat *> seats = m_shell->compositor()->seats();
        for (Out &o: candidates) {
            for (Seat *s: seats) {
                if (o.output->geometry().contains(s->pointer()->x(), s->pointer()->y())) {
                    o.vote++;
                }
            }
        }
        Out *out = nullptr;
        for (Out &o: candidates) {
            if (!out || out->vote < o.vote) {
                out = &o;
            }
        }
        output = out->output;
    }
    return output;
}

void ShellSurface::outputCreated(Output *o)
{
    ShellView *view = new ShellView(this, weston_view_create(m_surface));
    view->setDesignedOutput(o);

    if (View *v = *m_views.begin()) {
        view->setInitialPos(v->pos());
    }

    m_views.insert(o->id(), view);
    m_forceMap = true;
    configure(0, 0);
}

void ShellSurface::outputRemoved(Output *o)
{
    View *v = viewForOutput(o);
    m_views.remove(o->id());
    delete v;
}

ShellSurface *ShellSurface::fromSurface(weston_surface *s)
{
    if (s->configure == staticConfigure) {
        return static_cast<ShellSurface *>(s->configure_private);
    }
    return nullptr;
}

}
