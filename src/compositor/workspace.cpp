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

#include "workspace.h"
#include "shell.h"
#include "layer.h"
#include "view.h"
#include "shellsurface.h"
#include "output.h"
#include "compositor.h"
#include "dummysurface.h"
#include "shellview.h"

namespace Orbital {

Workspace::Workspace(Shell *shell)
         : Object(shell)
         , m_shell(shell)
{
    for (Output *o: shell->compositor()->outputs()) {
        WorkspaceView *view = new WorkspaceView(this, o, o->width(), o->height());
        m_views.insert(o->id(), view);
    }
}

Compositor *Workspace::compositor() const
{
    return m_shell->compositor();
}

WorkspaceView *Workspace::viewForOutput(Output *o)
{
    return m_views.value(o->id());
}

void Workspace::append(Layer *layer)
{
    for (WorkspaceView *v: m_views) {
        v->append(layer);
    }
}



WorkspaceView::WorkspaceView(Workspace *ws, Output *o, int w, int h)
             : m_workspace(ws)
             , m_output(o)
             , m_width(w)
             , m_height(h)
             , m_layer(new Layer)
             , m_attached(false)
{
    m_root = ws->compositor()->createDummySurface(0, 0);

    setPos(0, 0); //FIXME
}

void WorkspaceView::setPos(int x, int y)
{
    m_root->setPos(x, y);
    m_layer->setMask(x, y, m_width, m_height);
}

void WorkspaceView::attach(View *view, int x, int y)
{
    m_root->setTransformParent(view);
    m_layer->setMask(x, y, m_width, m_height);

    m_attached = true;
    emit m_workspace->activated(m_output);
}

void WorkspaceView::detach()
{
    setPos(0, 0);
    m_layer->setMask(0, 0, 0, 0);
    m_attached = false;
    emit m_workspace->deactivated(m_output);
}

bool WorkspaceView::isAttached() const
{
    return m_attached;
}

void WorkspaceView::append(Layer *layer)
{
    m_layer->append(layer);
}

void WorkspaceView::configure(View *view)
{
    m_layer->addView(view);
    view->setTransformParent(m_root);
}

void WorkspaceView::configureFullscreen(View *view, View *blackSurface)
{
    m_workspace->compositor()->rootLayer()->addView(blackSurface);
    m_workspace->compositor()->rootLayer()->addView(view);
    view->setTransformParent(m_root);
    blackSurface->setTransformParent(m_root);
}

}
