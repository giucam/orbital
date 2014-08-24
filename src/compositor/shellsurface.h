
#ifndef ORBITAL_SHELLSURFACE_H
#define ORBITAL_SHELLSURFACE_H

#include <QHash>
#include <QObject>

#include "interface.h"
#include "utils.h"

struct wl_client;
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
        Toplevel = 1,
        Popup
    };
    Q_DECLARE_FLAGS(States, State)

    ShellView *viewForOutput(Output *o);
    bool isMapped() const;
    void setWorkspace(Workspace *ws);
    Workspace *workspace() const;
    wl_client *client() const;

    State state() const;
    void setToplevel();
    void setPopup(weston_surface *parent, Seat *seat, int x, int y);
    void move(Seat *seat);

    static ShellSurface *fromSurface(weston_surface *s);

signals:
    void popupDone();

private:
    static void staticConfigure(weston_surface *s, int x, int y);
    void configure(int x, int y);
    void updateState();

    Shell *m_shell;
    weston_surface *m_surface;
    Workspace *m_workspace;
    QHash<int, ShellView *> m_views;

    State m_state;
    State m_nextState;

    weston_surface *m_parent;
    struct {
        int x;
        int y;
        Seat *seat;
    } m_popup;
};

}

DECLARE_OPERATORS_FOR_FLAGS(Orbital::ShellSurface::State)

#endif