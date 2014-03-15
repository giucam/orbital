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

#ifndef STYLEITEM_H
#define STYLEITEM_H

#include <QQuickItem>

#include "element.h"

class QQmlComponent;

class StyleComponent : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal leftContentsMargin READ leftContentsMargin WRITE setLeftContentsMargin NOTIFY contentsMarginsChanged)
    Q_PROPERTY(qreal topContentsMargin READ topContentsMargin WRITE setTopContentsMargin NOTIFY contentsMarginsChanged)
    Q_PROPERTY(qreal rightContentsMargin READ rightContentsMargin WRITE setRightContentsMargin NOTIFY contentsMarginsChanged)
    Q_PROPERTY(qreal bottomContentsMargin READ bottomContentsMargin WRITE setBottomContentsMargin NOTIFY contentsMarginsChanged)
    Q_PROPERTY(Element::Location location READ location NOTIFY locationChanged)
public:
    StyleComponent(QQuickItem *parent = nullptr);

    qreal leftContentsMargin() const { return m_leftMargin; }
    qreal topContentsMargin() const { return m_topMargin; }
    qreal rightContentsMargin() const { return m_rightMargin; }
    qreal bottomContentsMargin() const { return m_bottomMargin; }
    Element::Location location() const { return m_location; }

    void setLeftContentsMargin(qreal m) { m_leftMargin = m; emit contentsMarginsChanged(); }
    void setTopContentsMargin(qreal m) { m_topMargin = m; emit contentsMarginsChanged(); }
    void setRightContentsMargin(qreal m) { m_rightMargin = m; emit contentsMarginsChanged(); }
    void setBottomContentsMargin(qreal m) { m_bottomMargin = m; emit contentsMarginsChanged(); }

signals:
    void contentsMarginsChanged();
    void locationChanged();

private:
    qreal m_leftMargin;
    qreal m_topMargin;
    qreal m_rightMargin;
    qreal m_bottomMargin;
    Element::Location m_location;

    friend class StyleItem;
};

class StyleItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *component READ component WRITE setComponent)
    Q_PROPERTY(QQuickItem *item READ item NOTIFY itemChanged)
public:
    StyleItem(QQuickItem *p = nullptr);

    QQmlComponent *component() const { return m_component; }
    void setComponent(QQmlComponent *c);

    QQuickItem *item() const { return m_item; }
    void updateLocation(Element::Location loc);

protected:
    virtual void itemChange(ItemChange change, const ItemChangeData &value) override;
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

signals:
    void itemChanged();

private:
    void updateMargins();

    QQmlComponent *m_component;
    StyleComponent *m_item;
    bool m_acceptChildren;
    QQuickItem *m_child;
    qreal m_margins[4];

    friend class Element;
};

#endif
