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

#include <QQmlComponent>
#include <QtQml>
#include <QDebug>

#include "styleitem.h"

static const int a = qmlRegisterType<StyleItem>("Orbital", 1, 0, "StyleItem");
static const int b = qmlRegisterType<StyleComponent>("Orbital", 1, 0, "StyleComponent");

StyleItem::StyleItem(QQuickItem *p)
         : QQuickItem(p)
         , m_component(nullptr)
         , m_item(nullptr)
         , m_child(new QQuickItem(this))
{
    memset(m_margins, 0, 4 * sizeof(qreal));
}

void StyleItem::setComponent(QQmlComponent *c)
{
    QQuickItem *old = m_item;

    m_component = c;
    m_item = nullptr;
    if (c) {
        QObject *obj = c->beginCreate(c->creationContext());
        m_item = qobject_cast<StyleComponent *>(obj);
        if (m_item) {
            Element *e = Element::fromItem(parentItem());
            if (e) {
                m_item->m_location = e->location();
            }
        } else {
            qWarning() << "Style components must instantiate StyleComponent items.";
        }
        c->completeCreate();
    }

    if (m_item) {
        m_item->setParentItem(this);
        m_item->setWidth(width());
        m_item->setHeight(height());

        m_child->setParentItem(m_item);

        connect(m_item, &StyleComponent::contentsMarginsChanged, this, &StyleItem::updateMargins);
    } else {
        m_item = nullptr;
        m_child->setParentItem(this);
    }

    delete old;
    emit itemChanged();
    updateMargins();
}

void StyleItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuickItem::ItemChildAddedChange && value.item != m_child && value.item != m_item) {
        value.item->setParentItem(m_child);
    }

    QQuickItem::itemChange(change, value);
}

void StyleItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (m_item) {
        m_item->setWidth(newGeometry.width());
        m_item->setHeight(newGeometry.height());
    }
    m_child->setX(m_margins[0]);
    m_child->setY(m_margins[1]);
    m_child->setWidth(width() - m_margins[0] - m_margins[2]);
    m_child->setHeight(height() - m_margins[1] - m_margins[3]);

    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void StyleItem::updateMargins()
{
    if (!m_item) {
        memset(m_margins, 0, 4 * sizeof(qreal));
    } else {
        m_margins[0] = m_item->leftContentsMargin();
        m_margins[1] = m_item->topContentsMargin();
        m_margins[2] = m_item->rightContentsMargin();
        m_margins[3] = m_item->bottomContentsMargin();
    }

    m_child->setX(m_margins[0]);
    m_child->setY(m_margins[1]);
    m_child->setWidth(width() - m_margins[0] - m_margins[2]);
    m_child->setHeight(height() - m_margins[1] - m_margins[3]);
}

void StyleItem::updateLocation(Element::Location l)
{
    if (m_item) {
        m_item->m_location = l;
        emit m_item->locationChanged();
    }
}


StyleComponent::StyleComponent(QQuickItem *p)
              : QQuickItem(p)
              , m_leftMargin(0)
              , m_topMargin(0)
              , m_rightMargin(0)
              , m_bottomMargin(0)
              , m_location(Element::Location::Floating)
{
}
