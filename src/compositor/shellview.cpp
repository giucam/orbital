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
#include "transform.h"
#include "compositor.h"
#include "dummysurface.h"

namespace Orbital {

ShellView::ShellView(ShellSurface *surf, weston_view *view)
         : View(view)
         , m_surface(surf)
         , m_designedOutput(nullptr)
         , m_posSaved(false)
         , m_blackSurface(nullptr)
{

}

ShellView::~ShellView()
{
    delete m_blackSurface;
}

ShellSurface *ShellView::surface() const
{
    return m_surface;
}

void ShellView::setDesignedOutput(Output *o)
{
    m_designedOutput = o;
}

void ShellView::configureToplevel(bool map, bool maximized, bool fullscreen, int dx, int dy)
{
    if (map || dx || dy) {
        if (maximized) {
            QRect rect = m_designedOutput->availableGeometry();
            setPos(rect.topLeft());
            if (!m_posSaved) {
                m_savedPos = QPointF(x(), y());
                m_posSaved = true;
            }
        } else if (fullscreen) {
            if (!m_posSaved) {
                m_savedPos = QPointF(x(), y());
                m_posSaved = true;
            }
            mapFullscreen();
        } else {
            if (m_posSaved) {
                setPos(m_savedPos);
                m_posSaved = false;
            } else if (dx || dy) {
                setPos(x() + dx, y() + dy);
            } else if (!isMapped()) {
                setPos(20, 100);
            }
        }
    }

    if (map) {
        WorkspaceView *wsv = m_surface->workspace()->viewForOutput(m_designedOutput);
        if (fullscreen) {
            wsv->configureFullscreen(this, m_blackSurface);
        } else {
            if (m_blackSurface) {
                m_blackSurface->unmap();
            }
            wsv->configure(this);
        }
        setOutput(m_designedOutput);
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

void ShellView::cleanupAndUnmap()
{
    if (m_blackSurface) {
        m_blackSurface->unmap();
    }
    unmap();
}

void ShellView::mapFullscreen()
{
    const QRect rect = m_surface->surfaceTreeBoundingBox();
    const int sw = rect.width();
    const int sh = rect.height();
    const int ow = m_designedOutput->width();
    const int oh = m_designedOutput->height();

    if (ow == sw && oh == sh) {
        setPos(0, 0);
        return;
    }

    if (!m_blackSurface) {
        Compositor *c = m_surface->compositor();
        m_blackSurface = c->createDummySurface(m_designedOutput->width(), m_designedOutput->height());
    }

    double outputAspect = (double)ow / (double)oh;
    double surfaceAspect = (double)sw / (double)sh;
    double scale;
    if (outputAspect < surfaceAspect) {
        scale = (double)ow / (double)sw;
    } else {
        scale = (double)ow / (double)sh;
    }

    Transform tr;
    tr.scale(scale, scale);
    setTransform(tr);

    double x = (ow - sw * scale) / 2 - rect.x();
    double y = (oh - sh * scale) / 2 - rect.y();
    setPos(x, y);
}

}
