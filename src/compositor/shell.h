
#ifndef ORBITAL_SHELL_H
#define ORBITAL_SHELL_H

#include <functional>

#include "interface.h"
#include "seat.h"

namespace Orbital {

class Compositor;
class Layer;
class Workspace;
class ShellSurface;
enum class PointerCursor: unsigned int;

class Shell : public Object
{
    Q_OBJECT
public:
    typedef std::function<void (Pointer *, PointerCursor)> GrabCursorSetter;

    explicit Shell(Compositor *c);

    Compositor *compositor() const;
    void addWorkspace(Workspace *ws);
    QList<Workspace *> workspaces() const;

    void setGrabCursor(Pointer *pointer, PointerCursor c);
    void configure(ShellSurface *shsurf);

    void setGrabCursorSetter(GrabCursorSetter s);

private:
    Compositor *m_compositor;
    QList<Workspace *> m_workspaces;
    GrabCursorSetter m_grabCursorSetter;
};

}

#endif
