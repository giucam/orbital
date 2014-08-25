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

#include <QDebug>

#include <weston/compositor.h>

#include "shellview.h"
#include "shellsurface.h"
#include "output.h"
#include "workspace.h"

namespace Orbital {

ShellView::ShellView(ShellSurface *surf, weston_view *view)
         : View(view)
         , m_surface(surf)
         , m_designedOutput(nullptr)
         , m_posSaved(false)
         , m_maximized(false)
{
}

ShellView::~ShellView()
{
}

ShellSurface *ShellView::surface() const
{
    return m_surface;
}

void ShellView::setDesignedOutput(Output *o)
{
    m_designedOutput = o;
}

void ShellView::configureToplevel(bool maximized, int dx, int dy)
{
    bool updatePos = !isMapped() || m_maximized != maximized || dx || dy;
    m_maximized = maximized;

    if (!isMapped()) {
        WorkspaceView *wsv = m_surface->workspace()->viewForOutput(m_designedOutput);
        wsv->configure(this);
        setOutput(m_designedOutput);
    }

    if (updatePos) {
        QPointF pos;
        if (maximized) {
            QRect rect = m_designedOutput->availableGeometry();
            pos = rect.topLeft();
            if (!m_posSaved) {
                m_savedPos = QPointF(x(), y());
                m_posSaved = true;
            }
        } else {
            if (m_posSaved) {
                pos = m_savedPos;
                m_posSaved = false;
            } else if (dx || dy) {
                pos = QPointF(x() + dx, y() + dy);
            } else {
                pos = QPointF(20, 100);
            }
        }

        setPos(pos);
    }
    update();
}

void ShellView::configurePopup(ShellView *parent, int x, int y)
{
    if (!isMapped()) {
        WorkspaceView *wsv = m_surface->workspace()->viewForOutput(m_designedOutput);
        wsv->configure(this);
        setTransformParent(parent);
        setOutput(m_designedOutput);
        setPos(x, y);
    }
    update();
}

}