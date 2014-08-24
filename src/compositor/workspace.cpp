
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
    if (!view->isMapped()) {
        m_layer->addView(view);
        view->setTransformParent(m_root);
    }
}

}
