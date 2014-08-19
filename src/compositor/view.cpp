
#include <weston/compositor.h>

#include "view.h"
#include "output.h"
#include "workspace.h"

namespace Orbital {

View::View(weston_view *view)
    : m_view(view)
    , m_output(nullptr)
{
}

View::~View()
{
    weston_view_destroy(m_view);
}

bool View::isMapped() const
{
    return weston_view_is_mapped(m_view);
}

void View::setOutput(Output *o)
{
    m_output = o;
    m_view->output = o->m_output;
}

void View::setPos(int x, int y)
{
    weston_view_set_position(m_view, x, y);
}

void View::setTransformParent(View *p)
{
    weston_view_set_transform_parent(m_view, p->m_view);
    weston_view_update_transform(m_view);
}

void View::update()
{
    weston_view_geometry_dirty(m_view);
    weston_view_update_transform(m_view);
}

Output *View::output() const
{
    return m_output;
}

}
