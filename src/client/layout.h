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

#ifndef LAYOUT_H
#define LAYOUT_H

#include <QQuickItem>

class LayoutAttached;

class Layout : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing);
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation);
public:
    Layout(QQuickItem *parent = nullptr);

    Q_INVOKABLE void insertAt(QQuickItem *item, int col);
    Q_INVOKABLE void insertBefore(QQuickItem *item, QQuickItem *before);
    Q_INVOKABLE void insertAfter(QQuickItem *item, QQuickItem *after);
    Q_INVOKABLE void relayout();
    void invalidate();

    qreal spacing() const { return m_spacing; }
    void setSpacing(qreal spacing);

    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation);

    static LayoutAttached *qmlAttachedProperties(QObject *object);

protected:
    bool event(QEvent *e) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    QList<QQuickItem *> m_items;
    bool m_dirty;
    qreal m_spacing;
    Qt::Orientation m_orientation;
};

class LayoutAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged)
    Q_PROPERTY(qreal minimumHeight READ minimumHeight WRITE setMinimumHeight NOTIFY minimumHeightChanged)
    Q_PROPERTY(qreal preferredWidth READ preferredWidth WRITE setPreferredWidth NOTIFY preferredWidthChanged)
    Q_PROPERTY(qreal preferredHeight READ preferredHeight WRITE setPreferredHeight NOTIFY preferredHeightChanged)
    Q_PROPERTY(qreal maximumWidth READ maximumWidth WRITE setMaximumWidth NOTIFY maximumWidthChanged)
    Q_PROPERTY(qreal maximumHeight READ maximumHeight WRITE setMaximumHeight NOTIFY maximumHeightChanged)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight)
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth)
    Q_PROPERTY(int index READ index WRITE setIndex)

public:
    LayoutAttached(QObject *object);

    qreal minimumWidth() const {return m_minimumWidth; }
    void setMinimumWidth(qreal width);

    qreal minimumHeight() const { return m_minimumHeight; }
    void setMinimumHeight(qreal height);

    qreal preferredWidth() const { return m_preferredWidth; }
    void setPreferredWidth(qreal width);

    qreal preferredHeight() const { return m_preferredHeight; }
    void setPreferredHeight(qreal height);

    qreal maximumWidth() const { return m_maximumWidth; }
    void setMaximumWidth(qreal width);

    qreal maximumHeight() const { return m_maximumHeight; }
    void setMaximumHeight(qreal height);

    bool fillWidth() const { return m_fillWidth; }
    void setFillWidth(bool fill);

    bool fillHeight() const { return m_fillHeight; }
    void setFillHeight(bool fill);

    int index() const { return m_index; }
    void setIndex(int index);

signals:
    void minimumWidthChanged();
    void minimumHeightChanged();
    void preferredWidthChanged();
    void preferredHeightChanged();
    void maximumWidthChanged();
    void maximumHeightChanged();
    void fillWidthChanged();
    void fillHeightChanged();

private:
    void invalidateItem();
    Layout *parentLayout() const;

    QQuickItem *m_item;
    qreal m_minimumWidth;
    qreal m_minimumHeight;
    qreal m_preferredWidth;
    qreal m_preferredHeight;
    qreal m_maximumWidth;
    qreal m_maximumHeight;

    int m_index;

    unsigned m_fillWidth : 1;
    unsigned m_fillHeight : 1;

    friend class Layout;
};

inline LayoutAttached *attachedLayoutObject(QQuickItem *item, bool create = true)
{
    return static_cast<LayoutAttached *>(qmlAttachedPropertiesObject<Layout>(item, create));
}

QML_DECLARE_TYPE(Layout)
QML_DECLARE_TYPEINFO(Layout, QML_HAS_ATTACHED_PROPERTIES)

#endif
