/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ORBITAL_WORKSPACE_H
#define ORBITAL_WORKSPACE_H

#include <QHash>
#include <QRect>

#include "interface.h"
#include "transform.h"
#include "animation.h"

struct weston_surface;

namespace Orbital {

class Shell;
class Layer;
class View;
class ShellSurface;
class WorkspaceView;
class Output;
class Compositor;
class DummySurface;
class Pager;
class Surface;
class Root;
class Transform;

// Due to mask() being an int, we can have up to 31 workspaces
class Workspace : public Object
{
    Q_OBJECT
//     Q_PROPERTY(int x READ x WRITE setX)
//     Q_PROPERTY(int y READ y WRITE setY)
public:
    Workspace(Shell *shell, int id);
    ~Workspace();

    Compositor *compositor() const;
    Pager *pager() const;
    WorkspaceView *viewForOutput(Output *o);
    View *topView() const;

    int id() const;
    int mask() const;
    int x() const;
    int y() const;
    void setPos(int x, int y);

signals:
    void positionChanged(int x, int y);

private:
    void outputRemoved(Output *o);

    Shell *m_shell;
    int m_id;
    int m_x;
    int m_y;
    QHash<int, WorkspaceView *> m_views;

    friend Pager;
};

class WorkspaceView : public QObject
{
public:
    WorkspaceView(Workspace *ws, Output *o);
    ~WorkspaceView();

    void configure(View *view);
    void configureFullscreen(View *view, View *blackSurface);

    void setBackground(Surface *surface);
    void resetMask();
    void setMask(const QRect &rect);
    inline QRect mask() const { return m_mask; }
    void setTransform(const Transform &tf, bool animate);
    const Transform &transform() const;
    QPoint pos() const;
    QPoint logicalPos() const;
    QPointF map(double x, double y) const;

    bool ownsView(View *view) const;

    Workspace *workspace() const { return m_workspace; }

private:
    void setTransformParent(View *p);
    void updateAnim(double v);

    Workspace *m_workspace;
    Output *m_output;
    Layer *m_backgroundLayer;
    Layer *m_layer;
    Layer *m_fullscreenLayer;
    Root *m_root;
    View *m_background;
    QList<View *> m_views;
    bool m_attached;
    QRect m_mask;
    struct {
        Transform orig, target;
        Animation anim;
    } m_transformAnim;

    friend Pager;
    friend Workspace;
};

}

#endif
