
#ifndef ORBITAL_WORKSPACE_H
#define ORBITAL_WORKSPACE_H

#include <QObject>
#include <QHash>

namespace Orbital {

class Shell;
class Layer;
class View;
class ShellSurface;
class WorkspaceView;
class Output;
class Compositor;
class DummySurface;

class Workspace : public QObject
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

private:
    Shell *m_shell;
    int m_x;
    int m_y;
    QHash<int, WorkspaceView *> m_views;
};

class WorkspaceView
{
public:
    explicit WorkspaceView(Workspace *ws, int w, int h);

    void configure(View *view);
    void setPos(int x, int y);
    void append(Layer *layer);

private:
    Workspace *m_workspace;
    int m_width;
    int m_height;
    Layer *m_layer;
    DummySurface *m_root;
    QList<View *> m_views;
};

}

#endif
