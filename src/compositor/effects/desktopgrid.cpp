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

#include <linux/input.h>

#include <QDebug>

#include "../shell.h"
#include "../compositor.h"
#include "../binding.h"
#include "../global.h"
#include "../seat.h"
#include "../transform.h"
#include "../workspace.h"
#include "../output.h"
#include "../view.h"
#include "../pager.h"
#include "../animation.h"
#include "../animationcurve.h"
#include "../shellsurface.h"
#include "../layer.h"
#include "desktopgrid.h"

namespace Orbital {

class DesktopGrid::Grab : public PointerGrab
{
public:
    Grab() : moving(nullptr), moved(false) {}
    Workspace::View *workspace(Output *out, double x, double y)
    {
        for (Workspace *ws: shell->workspaces()) {
            Workspace::View *wsv = workspaceViewForOutput(ws, out);
            if (wsv->mask().contains(x, y)) {
                return wsv;
            }
        }
        return nullptr;
    }
    void focus() override
    {
        for (Output *out: desktopgrid->m_activeOutputs) {
            if (out->contains(pointer()->x(), pointer()->y())) {
                return;
            }
        }
        end();
    }
    void motion(uint32_t time, double x, double y) override
    {
        pointer()->move(x, y);

        if (moving) {
            if (!moved) {
                moved = QPointF(origMousePos - QPointF(x, y)).manhattanLength() > 2;
                if (moved) {
                    shell->compositor()->layer(Compositor::Layer::Apps)->addView(moving);
                }
            }
            if (moved) {
                ShellSurface *shsurf = ShellSurface::fromSurface(moving->surface());
                AbstractWorkspace *w = shsurf->workspace();
                Output *out = moving->output();
                AbstractWorkspace::View *wsv = workspaceViewForOutput(w, out);
                QPointF p = wsv->map(x, y) + QPointF(dx, dy);
                QRect surfaceGeometry = shsurf->geometry();

                QPointF br = p + surfaceGeometry.bottomRight();
                if (shell->snapPos(out, br, 20)) {
                    p = br - surfaceGeometry.bottomRight();
                }

                QPointF tl = p + surfaceGeometry.topLeft();
                if (shell->snapPos(out, tl, 20)) {
                    p = tl - surfaceGeometry.topLeft();
                }

                shsurf->moveViews(int(p.x()), int(p.y()));
            }
        }
    }
    void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
    {
        View *view = pointer()->pickView();
        if (state == Pointer::ButtonState::Pressed) {
            if (ShellSurface::fromSurface(view->surface())) {
                moving = view;
                moved = false;
                QPointF pp(pointer()->x(), pointer()->y());
                Workspace::View *wsv = workspace(moving->output(), pp.x(), pp.y());
                QPointF p = wsv->map(pp.x(), pp.y());
                dx = moving->x() - p.x();
                dy = moving->y() - p.y();
                origPos = moving->pos();
                origMousePos = pp;
            }
        } else if (moving && moved) {
            QPointF pp(pointer()->x(), pointer()->y());
            Workspace::View *wsv = workspace(moving->output(), pp.x(), pp.y());
            ShellSurface *shsurf = ShellSurface::fromSurface(moving->surface());
            if (wsv) {
                QPointF p = moving->mapToGlobal(QPointF(0,0));
                p = wsv->map(p.x(), p.y());

                shsurf->moveViews((int)p.x(), (int)p.y());
                shsurf->setWorkspace(wsv->workspace());
            } else {
                shsurf->moveViews(origPos.x(), origPos.y());
                shsurf->setWorkspace(shsurf->workspace());
            }
            moving = nullptr;
            moved = false;
        } else {
            moving = nullptr;
            Output *out = view->output();
            if (!out) {
                return;
            }
            if (Workspace::View *wsv = workspace(out, pointer()->x(), pointer()->y())) {
                desktopgrid->terminate(out, wsv->workspace());
            }
        }
    }
    void ended() override
    {
        delete this;
    }

    Shell *shell;
    DesktopGrid *desktopgrid;
    View *moving;
    bool moved;
    double dx, dy;
    QPointF origPos, origMousePos;
};

DesktopGrid::DesktopGrid(Shell *shell)
           : Effect(shell)
           , m_shell(shell)
{
    shell->addAction("Effects.ToggleDesktopGrid", [this](Seat *s) { run(s); });
    m_hsBinding = shell->compositor()->createHotSpotBinding(PointerHotSpot::TopRightCorner);
    connect(m_hsBinding, &HotSpotBinding::triggered, this, &DesktopGrid::runHotSpot);

    Compositor *c = m_shell->compositor();
    connect(c, &Compositor::outputCreated, this, &DesktopGrid::outputCreated);
    connect(c, &Compositor::outputRemoved, this, &DesktopGrid::outputRemoved);
    for (Output *out: c->outputs()) {
        connect(out, &Output::pointerEnter, this, &DesktopGrid::pointerEnter);
    }
    connect(m_shell->pager(), &Pager::workspaceActivated, this, &DesktopGrid::workspaceActivated);
}

DesktopGrid::~DesktopGrid()
{
}

void DesktopGrid::runKey(Seat *seat, uint32_t time, int key)
{
    run(seat);
}

void DesktopGrid::runHotSpot(Seat *seat, uint32_t time, PointerHotSpot hs)
{
    if (!seat->pointer()->isGrabActive()) {
        run(seat);
        return;
    }

    if (Grab *g = dynamic_cast<Grab *>(seat->pointer()->activeGrab())) {
        if (!g->moved) {
            run(seat);
        }
    }
}

void DesktopGrid::run(Seat *seat)
{
    Output *out = m_shell->selectPrimaryOutput(seat);

    int numWs = m_shell->workspaces().size();

    if (m_activeOutputs.contains(out)) {
        terminate(out, nullptr);
    } else {
        m_activeOutputs.insert(out);

        const int margin_w = out->width() / 20;
        const int margin_h = out->height() / 20;

        QRect fullRect;
        for (Workspace *w: m_shell->workspaces()) {
            Workspace::View *wsv = workspaceViewForOutput(w, out);
            QPoint p = wsv->logicalPos();
            p.rx() = (p.x() + 2) * margin_w + p.x() * out->geometry().width();
            p.ry() = (p.y() + 2) * margin_h + p.y() * out->geometry().height();
            QRect rect = QRect(p, out->geometry().size());
            fullRect |= rect;
        }
        fullRect.setTopLeft(QPoint());

        double rx = (double)out->width() / (double)fullRect.width();
        double ry = (double)out->height() / (double)fullRect.height();

        QSize fullSize;
        if (rx > ry) {
            rx = ry;
            double ratio = (double)out->width() / (double)out->height();
            fullSize = QSize(fullRect.height() * ratio, fullRect.height());
        } else {
            double ratio = (double)out->height() / (double)out->width();
            fullSize = QSize(fullRect.width(), fullRect.width() * ratio);
            ry = rx;
        }

        int margin_x = (fullSize.width() - fullRect.width()) / 2. * rx;
        int margin_y = (fullSize.height() - fullRect.height()) / 2. * rx;

        for (int i = 0; i < numWs; ++i) {
            Workspace *w = m_shell->workspaces().at(i);
            Workspace::View *wsv = workspaceViewForOutput(w, out);

            QPoint p = wsv->logicalPos();
            p.rx() = (p.x() * out->width() + (p.x() + 1) * margin_w) * rx;
            p.ry() = (p.y() * out->height() + (p.y() + 1) * margin_h) * rx;
            int x = p.x() - wsv->pos().x() + margin_x;
            int y = p.y() - wsv->pos().y() + margin_y;

            Transform tr;
            tr.scale(rx, rx);
            tr.translate(x, y);

            wsv->setTransform(tr, true);
        }

        Grab *grab = new Grab;
        grab->shell = m_shell;
        grab->desktopgrid = this;
        grab->start(seat, PointerCursor::Arrow);
    }
}

void DesktopGrid::terminate(Output *out, Workspace *ws)
{
    if (!ws) {
        ws = out->currentWorkspace();
    }
    m_activeOutputs.remove(out);
    m_shell->pager()->activate(ws, out);
}

void DesktopGrid::outputCreated(Output *o)
{
    connect(o, &Output::pointerEnter, this, &DesktopGrid::pointerEnter);
}

void DesktopGrid::outputRemoved(Output *o)
{
    m_activeOutputs.remove(o);
}

void DesktopGrid::pointerEnter(Pointer *p)
{
    Output *out = static_cast<Output *>(sender());
    if (m_activeOutputs.contains(out)) {
        Grab *grab = new Grab;
        grab->shell = m_shell;
        grab->desktopgrid = this;
        grab->start(p->seat(), PointerCursor::Arrow);
    }
}

void DesktopGrid::workspaceActivated(Workspace *ws, Output *out)
{
    m_activeOutputs.remove(out);
}

}
