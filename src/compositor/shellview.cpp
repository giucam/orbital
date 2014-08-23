
#include <weston/compositor.h>

#include "shellview.h"
#include "shellsurface.h"
#include "output.h"
#include "workspace.h"

namespace Orbital {

ShellView::ShellView(ShellSurface *surf, weston_view *view)
         : View(view)
         , m_surface(surf)
         , m_designedOutput(nullptr)
{
}

ShellView::~ShellView()
{
}

ShellSurface *ShellView::surface() const
{
    return m_surface;
}

void ShellView::setDesignedOutput(Output *o)
{
    m_designedOutput = o;
}

void ShellView::configure()
{
    if (m_surface->state() == ShellSurface::State::None) {
        return;
    }

    WorkspaceView *wsv = m_surface->workspace()->viewForOutput(m_designedOutput);
    wsv->configure(this);
    setOutput(m_designedOutput);
    setPos(0, 100);
    update();
}

}
