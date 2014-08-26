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
    void configureFullscreen(View *view, View *blackSurface);
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
