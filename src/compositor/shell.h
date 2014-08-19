
#ifndef ORBITAL_SHELL_H
#define ORBITAL_SHELL_H

#include "interface.h"

namespace Orbital {

class Compositor;
class Layer;
class Workspace;
class ShellSurface;

class Shell : public Object
{
    Q_OBJECT
public:
    explicit Shell(Compositor *c);

    Compositor *compositor() const;
    void addWorkspace(Workspace *ws);

    void configure(ShellSurface *shsurf);

private:
    Compositor *m_compositor;
    QList<Workspace *> m_workspaces;
};

}

#endif
