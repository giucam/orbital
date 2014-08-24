
#include <QDebug>

#include <weston/compositor.h>

#include "shellsurface.h"
#include "shell.h"
#include "shellview.h"
#include "workspace.h"
#include "output.h"
#include "compositor.h"
#include "seat.h"

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
    surface->configure = staticConfigure;

    for (Output *o: shell->compositor()->outputs()) {
        ShellView *view = new ShellView(this, weston_view_create(m_surface));
        view->setDesignedOutput(o);
        m_views.insert(o->id(), view);
    }

    connect(this, &ShellSurface::popupDone, [this]() { m_nextState = State::None; });
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

wl_client *ShellSurface::client() const
{
    return m_surface->resource ? wl_resource_get_client(m_surface->resource) : nullptr;
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

void ShellSurface::setPopup(weston_surface *parent, Seat *seat, int x, int y)
{
    m_parent = parent;
    m_popup.x = x;
    m_popup.y = y;
    m_popup.seat = seat;

    m_nextState = State::Popup;
}

void ShellSurface::move(Seat *seat)
{

    class MoveGrab : public PointerGrab
    {
    public:
        void motion(uint32_t time, double x, double y) override
        {
            pointer()->move(x, y);

            double moveX = x + dx;
            double moveY = y + dy;

            for (View *view: shsurf->m_views) {
                view->setPos(moveX, moveY);
            }
        }
        void button(uint32_t time, Pointer::Button button, Pointer::ButtonState state) override
        {
            if (pointer()->buttonCount() == 0 && state == Pointer::ButtonState::Released) {
    //             shsurf->moveEndSignal(shsurf);
    //             shsurf->m_runningGrab = nullptr;
                delete this;
            }
        }

        ShellSurface *shsurf;
        double dx, dy;
    };

    MoveGrab *move = new MoveGrab;

//     if (m_runningGrab) {
//         return;
//     }
//
//     if (m_type == ShellSurface::Type::TopLevel && m_state.fullscreen) {
//         return;
//     }

//     MoveGrab *move = new MoveGrab;
//     if (!move)
//         return;
//

    View *view = seat->pointer()->pickView();
    move->dx = view->x() - seat->pointer()->x();
    move->dy = view->y() - seat->pointer()->y();
    move->shsurf = this;
//     m_runningGrab = move;

    move->start(seat);
//     moveStartSignal(this);
}

void ShellSurface::staticConfigure(weston_surface *s, int32_t x, int32_t y)
{
    static_cast<ShellSurface *>(s->configure_private)->configure(x, y);
}

void ShellSurface::configure(int x, int y)
{
    updateState();

    if (m_state == State::None) {
        return;
    }

    m_shell->configure(this);

    if (m_state == State::Toplevel) {
        for (ShellView *view: m_views) {
            view->configureToplevel();
        }
    } else if (m_state == State::Popup) {
        ShellSurface *parent = ShellSurface::fromSurface(m_parent);
        if (!parent) {
            qWarning("Trying to map a popup without a ShellSurface parent.");
            return;
        }

        for (Output *o: m_shell->compositor()->outputs()) {
            ShellView *view = viewForOutput(o);
            ShellView *parentView = parent->viewForOutput(o);

            view->configurePopup(parentView, m_popup.x, m_popup.y);
        }
        m_popup.seat->grabPopup(this);
    }
    weston_surface_damage(m_surface);
}

void ShellSurface::updateState()
{
    m_state = m_nextState;
}

ShellSurface *ShellSurface::fromSurface(weston_surface *s)
{
    if (s->configure == staticConfigure) {
        return static_cast<ShellSurface *>(s->configure_private);
    }
    return nullptr;
}

}
