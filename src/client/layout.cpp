/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <QtQml>
#include <QDebug>

#include "layout.h"

static const int a = qmlRegisterType<Layout>("Orbital", 1, 0, "Layout");
static const int b = qmlRegisterType<LayoutAttached>();

LayoutAttached::LayoutAttached(QObject *object)
              : QObject(object)
              , m_item(qobject_cast<QQuickItem *>(object))
              , m_minimumWidth(0)
              , m_minimumHeight(0)
              , m_preferredWidth(0)
              , m_preferredHeight(0)
              , m_maximumWidth(10000)
              , m_maximumHeight(10000)
              , m_fillWidth(false)
              , m_fillHeight(false)
{
    QQmlProperty property(object, "layoutItem");
    property.write(QVariant::fromValue(this));
}

void LayoutAttached::setMinimumWidth(qreal width)
{
    m_minimumWidth = width;
    invalidateItem();
    emit minimumWidthChanged();
}

void LayoutAttached::setPreferredWidth(qreal width)
{
    m_preferredWidth = width;
    invalidateItem();
    emit preferredWidthChanged();
}

void LayoutAttached::setMaximumWidth(qreal width)
{
    m_maximumWidth = width;
    invalidateItem();
    emit maximumWidthChanged();
}

void LayoutAttached::setMinimumHeight(qreal height)
{
    m_minimumHeight = height;
    invalidateItem();
    emit minimumHeightChanged();
}

void LayoutAttached::setPreferredHeight(qreal height)
{
    m_preferredHeight = height;
    invalidateItem();
    emit preferredHeightChanged();
}

void LayoutAttached::setMaximumHeight(qreal height)
{
    m_maximumHeight = height;
    invalidateItem();
    emit maximumHeightChanged();
}

void LayoutAttached::setFillWidth(bool fill)
{
    m_fillWidth = fill;
    invalidateItem();
    emit fillWidthChanged();
}

void LayoutAttached::setFillHeight(bool fill)
{
    m_fillHeight = fill;
    invalidateItem();
    emit fillHeightChanged();
}

void LayoutAttached::setIndex(int index)
{
    if (Layout *l = parentLayout()) {
        l->insertAt(m_item, index);
    }
}

void LayoutAttached::invalidateItem()
{
    if (Layout *l = parentLayout()) {
        l->invalidate();
    }
}

Layout *LayoutAttached::parentLayout() const
{
    if (m_item) {
        QQuickItem *item = m_item->parentItem();
        return qobject_cast<Layout *>(item);
    } else {
        qWarning("Layout must be attached to Item elements");
    }
    return nullptr;
}



Layout::Layout(QQuickItem *p)
      : QQuickItem(p)
      , m_dirty(false)
      , m_spacing(0)
      , m_orientation(Qt::Horizontal)
{
}

LayoutAttached *Layout::qmlAttachedProperties(QObject *object)
{
    return new LayoutAttached(object);
}

bool Layout::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest) {
        relayout();
    }

    return QQuickItem::event(e);
}

void Layout::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    invalidate();
}

void Layout::itemChange(ItemChange change, const ItemChangeData &value)
{
    switch (change) {
        case QQuickItem::ItemChildAddedChange:
            m_items << value.item;
            invalidate();
            break;
        case QQuickItem::ItemChildRemovedChange:
            m_items.removeOne(value.item);
            invalidate();
            break;
        default:
            break;
    }

    QQuickItem::itemChange(change, value);
}

void Layout::insertAt(QQuickItem *item, int col)
{
    if (item->parentItem() != this) {
        item->setParentItem(this);
    }

    if (m_items.contains(item)) {
        m_items.removeOne(item);
    }
    m_items.insert(col, item);

    invalidate();
}

void Layout::insertBefore(QQuickItem *item, QQuickItem *before)
{
    if (item->parentItem() != this) {
        item->setParentItem(this);
    }

    if (m_items.contains(item)) {
        m_items.removeOne(item);
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i) == before) {
            m_items.insert(i, item);
            break;
        }
    }

    invalidate();
}

void Layout::insertAfter(QQuickItem *item, QQuickItem *after)
{
    if (item->parentItem() != this) {
        item->setParentItem(this);
    }

    if (m_items.contains(item)) {
        m_items.removeOne(item);
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i) == after) {
            m_items.insert(i + 1, item);
            break;
        }
    }

    invalidate();
}

void Layout::invalidate()
{
    for (int i = 0; i < m_items.size(); ++i) {
        QQuickItem *item = m_items.at(i);
        LayoutAttached *la = attachedLayoutObject(item);
        la->m_index = i;
    }

    if (m_dirty)
        return;

    m_dirty = true;
    QCoreApplication::postEvent(this, new QEvent(QEvent::LayoutRequest));
}

void Layout::setSpacing(qreal spacing)
{
    m_spacing = spacing;
    invalidate();
}

void Layout::setOrientation(Qt::Orientation orientation)
{
    m_orientation = orientation;
    invalidate();
}

void Layout::relayout()
{
    m_dirty = false;

    if (m_orientation == Qt::Vertical) {
        qWarning("Layout: vertical orientation not implemented.");
    }

    int expandables = 0;
    qreal x = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        QQuickItem *item = m_items.at(i);
        item->setX(x);
        item->setY(0);
        LayoutAttached *la = attachedLayoutObject(item);
        qreal w = la->preferredWidth();
        item->setWidth(w);
        item->setHeight(height());

        if (la->fillWidth()) {
            ++expandables;
        }

        x += w + m_spacing;
    }

    qreal spaceLeft = width() - x;
    if (spaceLeft > 0) {
        spaceLeft /= expandables;
        x = 0;
        for (QQuickItem *i: m_items) {
            LayoutAttached *la = attachedLayoutObject(i);
            i->setX(x);
            if (la->fillWidth()) {
                qreal w = i->width() + spaceLeft;
                if (w > la->maximumWidth()) {
                    w = la->maximumWidth();
                }
                i->setWidth(w);
            }

            x += i->width();
        }
    } else if (spaceLeft < 0) {
        spaceLeft /= m_items.count();
        x = 0;
        for (QQuickItem *i: m_items) {
            LayoutAttached *la = attachedLayoutObject(i);
            i->setX(x);

            qreal w = i->width() + spaceLeft;
            if (w < la->minimumWidth()) {
                w = la->minimumWidth();
            }
            i->setWidth(w);

            x += w + m_spacing;
        }
    }
}
