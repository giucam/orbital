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

#include <signal.h>
#include <unistd.h>

#include <QDebug>

#include <compositor.h>

#include "shellsurface.h"
#include "shell.h"
#include "shellview.h"
#include "workspace.h"
#include "output.h"
#include "compositor.h"
#include "seat.h"
#include "pager.h"
#include "layer.h"
#include "format.h"
#include "surface.h"

namespace Orbital
{

std::unordered_map<std::string, QPoint> ShellSurface::s_posCache;

ShellSurface::ShellSurface(Shell *shell, Surface *surface, Handler h)
            : Object()
            , m_shell(shell)
            , m_surface(surface)
            , m_handler(std::move(h))
            , m_workspace(nullptr)
            , m_previewView(nullptr)
            , m_resizeEdges(Edges::None)
            , m_forceMap(false)
            , m_currentGrab(nullptr)
            , m_isResponsive(true)
            , m_type(Type::None)
            , m_nextType(Type::None)
            , m_parent(nullptr)
            , m_toplevel({ false, false, nullptr })
            , m_transient({ 0, 0, false })
            , m_state({ QSize(), false, false })
{
    surface->setMoveHandler([this](Seat *seat) { move(seat); });
    surface->setShellSurface(this);

    for (Output *o: shell->compositor()->outputs()) {
        viewForOutput(o);
    }
    connect(shell->compositor(), &Compositor::outputCreated, this, &ShellSurface::outputCreated);
    connect(shell->compositor(), &Compositor::outputRemoved, this, &ShellSurface::outputRemoved);
    connect(shell->pager(), &Pager::workspaceActivated, this, &ShellSurface::workspaceActivated);

    wl_client_get_credentials(surface->client(), &m_pid, NULL, NULL);
}

ShellSurface::~ShellSurface()
{
    delete m_previewView;
    delete m_currentGrab;
    while (!m_views.empty()) {
        delete m_views.begin()->second;
    }
}

void ShellSurface::setHandler(Handler hnd)
{
    m_handler = std::move(hnd);
}

ShellView *ShellSurface::viewForOutput(Output *o)
{
    auto *view = m_views[o->id()];
    if (!view) {
        view = new ShellView(this);
        view->setDesignedOutput(o);
        m_views[o->id()] = view;
        connect(o, &Output::availableGeometryChanged, this, &ShellSurface::availableGeometryChanged);
        connect(view, &QObject::destroyed, this, [this, o] {
            m_views.erase(o->id());
        });
    }

    return view;
}

void ShellSurface::setWorkspace(AbstractWorkspace *ws)
{
    m_workspace = ws;
    m_surface->setWorkspaceMask(ws->mask());
    m_forceMap = true;
    committed(0, 0);
}

Compositor *ShellSurface::compositor() const
{
    return m_shell->compositor();
}

AbstractWorkspace *ShellSurface::workspace() const
{
    return m_workspace;
}

void ShellSurface::setToplevel()
{
    m_nextType = Type::Toplevel;
    m_toplevel.maximized = false;
    m_toplevel.fullscreen = false;
    disconnectParent();
}

void ShellSurface::setParent(Surface *parent, int x, int y, bool inactive)
{
    m_parent = parent;
    m_transient.x = x;
    m_transient.y = y;
    m_transient.inactive = inactive;

    disconnectParent();
    m_parentConnections.push_back(connect(m_parent, &QObject::destroyed, this, &ShellSurface::parentSurfaceDestroyed));
    m_nextType = Type::Transient;
}

void ShellSurface::connectParent()
{
    disconnectParent();
    m_parentConnections.push_back(connect(m_parent, &QObject::destroyed, this, &ShellSurface::parentSurfaceDestroyed));
    m_parentConnections.push_back(connect(m_surface, &Surface::activated, m_parent, &Surface::activated));
    m_parentConnections.push_back(connect(m_surface, &Surface::deactivated, m_parent, &Surface::deactivated));
}

void ShellSurface::disconnectParent()
{
    for (auto &c: m_parentConnections) {
        disconnect(c);
    }
    m_parentConnections.clear();
}

void ShellSurface::setMaximized()
{
    m_nextType = Type::Toplevel;
    m_toplevel.maximized = true;
    m_toplevel.fullscreen = false;
    m_toplevel.output = selectOutput();

    QRect rect = m_toplevel.output->availableGeometry();
    qDebug() << "Maximizing surface on output" << m_toplevel.output << "with rect" << rect;
    sendConfigure(rect.width(), rect.height());
}

void ShellSurface::setFullscreen()
{
    m_nextType = Type::Toplevel;
    m_toplevel.fullscreen = true;
    m_toplevel.maximized = false;

    Output *output = selectOutput();

    QRect rect = output->geometry();
    qDebug() << "Fullscrening surface on output" << output << "with rect" << rect;
    sendConfigure(rect.width(), rect.height());
}

void ShellSurface::move(Seat *seat)
{
    if (isFullscreen()) {
        return;
    }

    class MoveGrab : public PointerGrab
    {
    public:
        void motion(uint32_t time, Pointer::MotionEvent evt) override
        {
            pointer()->move(evt);
            QPointF pos = pointer()->motionToAbs(evt);

            Output *out = grabbedView->output();
            QRect surfaceGeometry = shsurf->geometry();

            int moveX = pos.x() + dx;
            int moveY = pos.y() + dy;

            QPointF p = QPointF(moveX, moveY);

            QPointF br = p + surfaceGeometry.bottomRight();
            if (shsurf->m_shell->snapPos(out, br)) {
                p = br - surfaceGeometry.bottomRight();
            }

            QPointF tl = p + surfaceGeometry.topLeft();
            if (shsurf->m_shell->snapPos(out, tl)) {
                p = tl - surfaceGeometry.topLeft();
            }

            shsurf->moveViews((int)p.x(), (int)p.y());
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            if (pointer()->buttonCount() == 0 && state == Pointer::ButtonState::Released) {
                end();
            }
        }
        void ended() override
        {
            shsurf->m_currentGrab = nullptr;
            delete this;
        }

        ShellSurface *shsurf;
        View *grabbedView;
        double dx, dy;
    };

    MoveGrab *move = new MoveGrab;

    View *view = seat->pointer()->pickView()->mainView();
    move->dx = view->x() - seat->pointer()->x();
    move->dy = view->y() - seat->pointer()->y();
    move->shsurf = this;
    move->grabbedView = view;

    move->start(seat, PointerCursor::Move);
    m_currentGrab = move;
}

void ShellSurface::resize(Seat *seat, Edges edges)
{
    class ResizeGrab : public PointerGrab
    {
    public:
        void motion(uint32_t time, Pointer::MotionEvent evt) override
        {
            pointer()->move(evt);
            QPointF pos = pointer()->motionToAbs(evt);

            QPointF from = view->mapFromGlobal(pointer()->grabPos());
            QPointF to = view->mapFromGlobal(pos);
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
            shsurf->m_currentGrab = nullptr;
            delete this;
        }

        ShellSurface *shsurf;
        View *view;
        int32_t width, height;
    };

    int e = (int)edges;
    if (e == 0 || e > 15 || (e & 3) == 3 || (e & 12) == 12) {
        return;
    }

    m_resizeEdges = edges;


    QRect rect = geometry();
    ResizeGrab *grab = new ResizeGrab;
    grab->width = m_width = rect.width();
    grab->height = m_height = rect.height();
    grab->shsurf = this;
    grab->view = seat->pointer()->pickView()->mainView();

    grab->start(seat, (PointerCursor)e);
    m_currentGrab = grab;
}

void ShellSurface::setBusyCursor(Pointer *pointer)
{
    if (m_isResponsive) {
        return;
    }

    class BusyGrab : public PointerGrab
    {
    public:
        void focus() override
        {
            View *view = pointer()->pickView();
            if (view->surface() != shsurf->surface()) {
                end();
            }
        }
        void motion(uint32_t time, Pointer::MotionEvent evt) override
        {
            pointer()->move(evt);
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
        }
        void ended() override
        {
            shsurf->m_currentGrab = nullptr;
            delete this;
        }

        ShellSurface *shsurf;
    };

    BusyGrab *grab = new BusyGrab;

    grab->shsurf = this;

    grab->start(pointer->seat(), PointerCursor::Busy);
    m_currentGrab = grab;
}

void ShellSurface::unmap()
{
    for (auto &i: m_views) {
        i.second->cleanupAndUnmap();
    }
    emit m_surface->unmapped();
}

void ShellSurface::minimize()
{
    unmap();
    emit minimized();
}

void ShellSurface::restore()
{
    m_forceMap = true;
    committed(0, 0);
    emit restored();
}

void ShellSurface::close()
{
    pid_t pid;
    wl_client_get_credentials(m_surface->client(), &pid, NULL, NULL);

    if (pid != getpid()) {
        kill(pid, SIGTERM);
    }
}

void ShellSurface::preview(Output *output)
{
    ShellView *v = viewForOutput(output);

    if (!m_previewView) {
        m_previewView = new ShellView(this);
        connect(m_previewView, &QObject::destroyed, this, [this]() { m_previewView = nullptr; });
        m_previewView->setActivatable(false);
    }

    m_previewView->setDesignedOutput(output);
    m_previewView->setPos(v->x(), v->y());

    m_shell->compositor()->layer(Compositor::Layer::Dashboard)->addView(m_previewView);
    m_previewView->setTransformParent(output->rootView());
    m_previewView->setAlpha(0.);
    m_previewView->animateAlphaTo(0.8);
}

void ShellSurface::endPreview(Output *output)
{
    if (m_previewView) {
        m_previewView->animateAlphaTo(0., [this]() { m_previewView->unmap(); });
    }
}

void ShellSurface::moveViews(double x, double y)
{
    s_posCache[cacheId()] = QPoint(x, y);
    for (auto &i: m_views) {
        i.second->move(QPointF(x, y));
    }
}

void ShellSurface::setTitle(StringView t)
{
    if (m_title != t) {
        m_title = t.toStdString();
        emit titleChanged();
        m_surface->setLabel(t);
    }
}

void ShellSurface::setAppId(StringView id)
{
    if (m_appId != id) {
        m_appId = id.toStdString();
        emit appIdChanged();
    }
}

void ShellSurface::setPid(pid_t pid)
{
    m_pid = pid;
}

void ShellSurface::setIsResponsive(bool responsive)
{
    if (responsive == m_isResponsive) {
        return;
    }

    m_isResponsive = responsive;
    // end the busy grab
    if (responsive && m_currentGrab) {
        m_currentGrab->end();
    }
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
    if (m_handler) {
        return m_handler.geometry();
    }
    return m_surface->boundingBox();
}

StringView ShellSurface::title() const
{
    return m_title;
}

StringView ShellSurface::appId() const
{
    return m_appId;
}

inline std::string ShellSurface::cacheId() const
{
    return fmt::format("{}+{}", m_appId, m_title);
}

Maybe<QPoint> ShellSurface::cachedPos() const
{
    std::string id = cacheId();
    auto it = s_posCache.find(id);
    return it != s_posCache.end() ? Maybe<QPoint>(it->second) : Maybe<QPoint>();
}

void ShellSurface::parentSurfaceDestroyed()
{
    m_parent = nullptr;
    m_nextType = Type::None;
}

void ShellSurface::committed(int x, int y)
{
    if (m_surface->width() == 0) {
        m_type = Type::None;
        m_workspace = nullptr;
        m_surface->unmap();
        emit contentLost();
        emit m_surface->unmapped();
        return;
    }

    updateState();

    if (m_type == Type::None) {
        return;
    }

    m_surface->setActivable(m_type != Type::Transient || !m_transient.inactive);

    bool wasMapped = m_surface->isMapped();
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

        bool map = m_state.maximized != m_toplevel.maximized || m_state.fullscreen != m_toplevel.fullscreen ||
                   m_state.size != rect.size() || m_forceMap;
        m_forceMap = false;
        m_state.size = rect.size();
        m_state.maximized = m_toplevel.maximized;
        m_state.fullscreen = m_toplevel.fullscreen;

        for (auto &i: m_views) {
            i.second->configureToplevel(map || !i.second->layer(), m_toplevel.maximized, m_toplevel.fullscreen, dx, dy);
        }
    } else if (m_type == Type::Transient) {
        if (!m_parent->shellSurface()) {
            View *parentView = View::fromView(wl_container_of(m_parent->surface()->views.next, (weston_view *)nullptr, surface_link));
            ShellView *view = viewForOutput(parentView->output());
            view->configureTransient(parentView, m_transient.x, m_transient.y);
        } else {
            for (Output *o: m_shell->compositor()->outputs()) {
                ShellView *view = viewForOutput(o);
                ShellView *parentView = m_parent->shellSurface()->viewForOutput(o);

                view->configureTransient(parentView, m_transient.x, m_transient.y);
            }
        }
    }
    m_surface->damage();
    m_surface->map();

    for (auto pair: m_views) {
        pair.second->update();
    }

    if (!wasMapped && m_surface->isMapped()) {
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
    if (m_handler) {
        m_handler.setSize(w, h);
    }
}

Output *ShellSurface::selectOutput()
{
    struct Out {
        Output *output;
        int vote;
    };
    std::vector<Out> candidates;
    for (Output *o: m_shell->compositor()->outputs()) {
        candidates.push_back({ o, m_shell->pager()->isWorkspaceActive(m_workspace, o) ? 10 : 0 });
    }

    Output *output = nullptr;
    if (candidates.empty()) {
        return nullptr;
    } else if (candidates.size() == 1) {
        output = candidates.front().output;
    } else {
        std::vector<Seat *> seats = m_shell->compositor()->seats();
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
    ShellView *view = viewForOutput(o);

    if (View *v = m_views.begin()->second) {
        view->setInitialPos(v->pos());
    }

    m_forceMap = true;
    committed(0, 0);
}

void ShellSurface::outputRemoved(Output *o)
{
    View *v = viewForOutput(o);
    m_views.erase(o->id());
    delete v;

    if (m_nextType == Type::Toplevel && m_toplevel.maximized && m_toplevel.output == o) {
        setMaximized();
    }
}

void ShellSurface::availableGeometryChanged()
{
    Output *o = static_cast<Output *>(sender());
    if (m_nextType == Type::Toplevel && m_toplevel.maximized && m_toplevel.output == o) {
        QRect rect = o->availableGeometry();
        sendConfigure(rect.width(), rect.height());
    }
}

void ShellSurface::workspaceActivated(Workspace *w, Output *o)
{
    if (w != workspace()) {
        return;
    }

    if (m_nextType != Type::Toplevel || !m_toplevel.maximized) {
        return;
    }

    if (m_toplevel.output->currentWorkspace() != w) {
        setMaximized();
    }
}

}
