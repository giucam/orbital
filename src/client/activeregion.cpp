/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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
#include <QQuickWindow>

#include "activeregion.h"
#include "client.h"

#include "wayland-desktop-shell-client-protocol.h"

ActiveRegion::ActiveRegion(QQuickItem *p)
            : QQuickItem(p)
            , m_activeRegion(nullptr)
{
    connect(this, &QQuickItem::windowChanged, this, &ActiveRegion::init, Qt::QueuedConnection);
}

ActiveRegion::~ActiveRegion()
{
    if (m_activeRegion) {
        active_region_destroy(m_activeRegion);
    }
}

void ActiveRegion::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (m_activeRegion) {
        QRect rect = mapRectToScene(QRectF(x(), y(), width(), height())).toRect();
        active_region_set_geometry(m_activeRegion, rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void ActiveRegion::init()
{
    if (m_activeRegion) {
        active_region_destroy(m_activeRegion);
        m_activeRegion = nullptr;
    }

    if (window()) {
        QRect rect = mapRectToScene(QRectF(x(), y(), width(), height())).toRect();
        m_activeRegion = Client::client()->createActiveRegion(window(), rect);
    }
}
