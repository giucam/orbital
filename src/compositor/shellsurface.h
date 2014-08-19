
#ifndef ORBITAL_SHELLSURFACE_H
#define ORBITAL_SHELLSURFACE_H

#include <QHash>
#include <QObject>

#include "interface.h"
#include "utils.h"

struct weston_surface;

namespace Orbital
{

class Shell;
class ShellView;
class Workspace;
class Output;
class Seat;

class ShellSurface : public Object
{
    Q_OBJECT
public:
    ShellSurface(Shell *shell, weston_surface *surface);
    ~ShellSurface();

    enum class State {
        None = 0,
        Toplevel = 1
    };
    Q_DECLARE_FLAGS(States, State)

    ShellView *viewForOutput(Output *o);
    bool isMapped() const;
    void setWorkspace(Workspace *ws);
    Workspace *workspace() const;

    State state() const;
    void setToplevel();
    void move(Seat *seat, uint32_t serial);

private:
    void configure(int x, int y);
    void updateState();

    Shell *m_shell;
    weston_surface *m_surface;
    Workspace *m_workspace;
    QHash<int, ShellView *> m_views;

    State m_state;
    State m_nextState;
};

}

DECLARE_OPERATORS_FOR_FLAGS(Orbital::ShellSurface::State)

#endif