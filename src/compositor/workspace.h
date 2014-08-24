
#ifndef ORBITAL_WORKSPACE_H
#define ORBITAL_WORKSPACE_H

#include <QHash>

#include "interface.h"

namespace Orbital {

class Shell;
class Layer;
class View;
class ShellSurface;
class WorkspaceView;
class Output;
class Compositor;
class DummySurface;

class Workspace : public Object
{
    Q_OBJECT
//     Q_PROPERTY(int x READ x WRITE setX)
//     Q_PROPERTY(int y READ y WRITE setY)
public:
    Workspace(Shell *shell);

    Compositor *compositor() const;
    void addSurface(ShellSurface *shsurf);
    WorkspaceView *viewForOutput(Output *o);
    void append(Layer *layer);

    int x() const;
    int y() const;
    void setX(int x);
    void setY(int y);

signals:
    void activated(Output *output);
    void deactivated(Output *output);

private:
    Shell *m_shell;
    int m_x;
    int m_y;
    QHash<int, WorkspaceView *> m_views;
};

class WorkspaceView
{
public:
    explicit WorkspaceView(Workspace *ws, Output *o, int w, int h);

    void configure(View *view);
    void append(Layer *layer);

    void attach(View *view, int x, int y);
    void detach();
    bool isAttached() const;

private:
    void setPos(int x, int y);

    Workspace *m_workspace;
    Output *m_output;
    int m_width;
    int m_height;
    Layer *m_layer;
    DummySurface *m_root;
    QList<View *> m_views;
    bool m_attached;
};

}

#endif
