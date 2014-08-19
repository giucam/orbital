
#include <weston/compositor.h>

#include "output.h"
#include "workspace.h"

namespace Orbital {

Output::Output(weston_output *out)
      : QObject()
      , m_output(out)
{

}

void Output::viewWorkspace(Workspace *ws)
{
    WorkspaceView *view = ws->viewForOutput(this);
    view->setPos(m_output->x, m_output->y);
}

int Output::id() const
{
    return m_output->id;
}

int Output::width() const
{
    return m_output->width;
}

int Output::height() const
{
    return m_output->height;
}

}
