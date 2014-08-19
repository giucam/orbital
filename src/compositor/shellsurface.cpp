
#include <QDebug>

#include <weston/compositor.h>

#include "shellsurface.h"
#include "shell.h"
#include "shellview.h"
#include "workspace.h"
#include "output.h"
#include "compositor.h"

namespace Orbital
{

ShellSurface::ShellSurface(Shell *shell, weston_surface *surface)
            : Object(shell)
            , m_shell(shell)
            , m_surface(surface)
            , m_state(State::None)
            , m_nextState(State::None)
{
    surface->configure_private = this;
    surface->configure = [](weston_surface *s, int32_t x, int32_t y) {
        static_cast<ShellSurface *>(s->configure_private)->configure(x, y);
    };

    for (Output *o: shell->compositor()->outputs()) {
        ShellView *view = new ShellView(this, weston_view_create(m_surface));
        view->setDesignedOutput(o);
        m_views.insert(o->id(), view);
    }
}

ShellSurface::~ShellSurface()
{

}

ShellView *ShellSurface::viewForOutput(Output *o)
{
    return m_views.value(o->id());
}

void ShellSurface::setWorkspace(Workspace *ws)
{
    m_workspace = ws;
}

Workspace *ShellSurface::workspace() const
{
    return m_workspace;
}

bool ShellSurface::isMapped() const
{
    return weston_surface_is_mapped(m_surface);
}

ShellSurface::State ShellSurface::state() const
{
    return m_state;
}

void ShellSurface::setToplevel()
{
    m_nextState = State::Toplevel;
}

void ShellSurface::move(Seat *seat, uint32_t serial)
{
    qDebug()<<"move"<<seat<<serial;
}

void ShellSurface::configure(int x, int y)
{
    updateState();

//     qDebug()<<"configure"<<this<<x<<y;
    m_shell->configure(this);
    for (ShellView *view: m_views) {
        view->configure();
    }
    weston_surface_damage(m_surface);
}

void ShellSurface::updateState()
{
    m_state = m_nextState;
}

}
