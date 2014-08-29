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

#ifndef ORBITAL_SHELLVIEW_H
#define ORBITAL_SHELLVIEW_H

#include <QPoint>

#include "view.h"

namespace Orbital {

class ShellSurface;
class Output;
class Layer;
class DummySurface;
class WorkspaceView;

class ShellView : public View
{
public:
    explicit ShellView(ShellSurface *shsurf, weston_view *view);
    ~ShellView();

    ShellSurface *surface() const;

    void setDesignedOutput(Output *o);
    void configureToplevel(bool map, bool maximized, bool fullscreen, int dx, int dy);
    void configurePopup(ShellView *parent, int x, int y);
    void configureTransient(ShellView *parent, int x, int y);

    void cleanupAndUnmap();

private:
    void mapFullscreen();

    ShellSurface *m_surface;
    Output *m_designedOutput;
    QPointF m_savedPos;
    bool m_posSaved;
    DummySurface *m_blackSurface;
};

}

#endif
