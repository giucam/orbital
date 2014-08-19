
#include "shell.h"
#include "compositor.h"
#include "layer.h"
#include "workspace.h"
#include "shellsurface.h"

#include "wlshell/wlshell.h"

namespace Orbital {

Shell::Shell(Compositor *c)
     : Object()
     , m_compositor(c)
{
    addInterface(new WlShell(this, m_compositor));
}

Compositor *Shell::compositor() const
{
    return m_compositor;
}

void Shell::addWorkspace(Workspace *ws)
{
    ws->append(m_compositor->rootLayer());
    m_workspaces << ws;
}

void Shell::configure(ShellSurface *shsurf)
{
    if (!shsurf->isMapped()) {
        shsurf->setWorkspace(m_workspaces.first());
    }
}

}
