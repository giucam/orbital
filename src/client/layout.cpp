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
              , m_orientation(Qt::Horizontal)
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

void LayoutAttached::setOrientation(Qt::Orientation o)
{
    if (m_orientation != o) {
        m_orientation = o;
        emit orientationChanged();
    }
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
        la->setOrientation(m_orientation);
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
    if (orientation != m_orientation) {
        m_orientation = orientation;
        invalidate();
    }
}

void Layout::relayout()
{
    m_dirty = false;

    struct It {
        QQuickItem *item;
        LayoutAttached *la;
        bool resize;
        qreal x;
        qreal w;
    };
    QList<It> _items;
    for (QQuickItem *i: m_items) {
        _items.append({ i, attachedLayoutObject(i), true, 0, 0 });
    }

    const bool horizontal = m_orientation == Qt::Horizontal;
    qreal x = 0;
    for (It &i: _items) {
        i.x = x;
        i.w = horizontal ? i.la->minimumWidth() : i.la->minimumHeight();

        x += i.w + m_spacing;
    }

    bool again;
    int num = _items.count();
    do {
        again = false;
        qreal spaceLeft = (horizontal ? width() : height()) - x;
        if (spaceLeft <= 0) {
            break;
        }

        spaceLeft /= num;
        x = 0;
        for (It &i: _items) {
            i.x = x;
            if (i.resize) {
                i.w += spaceLeft;
                qreal max = horizontal ? i.la->maximumWidth() : i.la->maximumHeight();
                qreal pref = horizontal ? i.la->preferredWidth() : i.la->preferredHeight();
                if ((horizontal ? i.la->fillWidth() : i.la->fillHeight())) {
                    if (max < i.w) {
                        i.w = max;
                        i.resize = false;
                        --num;
                        again = true;
                    } else if (spaceLeft > 1) {
                        again = true;
                    }
                } else if (pref < i.w) {
                    i.w = pref;
                    i.resize = false;
                    --num;
                    again = true;
                }
            }
            x += i.w + m_spacing;
        }
    } while (again);

    if (horizontal) {
        for (It &i: _items) {
            i.item->setX(i.x);
            i.item->setY(0);
            i.item->setWidth(i.w);
            i.item->setHeight(height());
        }
    } else {
        for (It &i: _items) {
            i.item->setY(i.x);
            i.item->setX(0);
            i.item->setHeight(i.w);
            i.item->setWidth(width());
        }
    }
}
