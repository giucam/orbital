
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
         : QObject(shell)
         , m_shell(shell)
{
    for (Output *o: shell->compositor()->outputs()) {
        WorkspaceView *view = new WorkspaceView(this, o->width(), o->height());
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



WorkspaceView::WorkspaceView(Workspace *ws, int w, int h)
             : m_workspace(ws)
             , m_layer(new Layer)
{
    m_root = ws->compositor()->createDummySurface(w, h);
    m_layer->addView(m_root);

    setPos(-10000, 0); //FIXME
}

void WorkspaceView::setPos(int x, int y)
{
    m_root->setPos(x, y);
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
