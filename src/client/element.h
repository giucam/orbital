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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ELEMENT_H
#define ELEMENT_H

#include <QQuickItem>
#include <QStringList>

class QQmlEngine;
class QQuickWindow;

struct wl_surface;

class LayoutAttached;

class Element : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Type type READ type WRITE setType)
    Q_PROPERTY(LayoutAttached *layoutItem READ layout WRITE setLayout)
    Q_PROPERTY(QString sortProperty READ sortProperty WRITE setSortProperty)
public:
    enum Type {
        Item,
        Background,
        Panel,
        Overlay
    };
    Q_ENUMS(Type)
    explicit Element(Element *parent = nullptr);
    ~Element();

    Q_INVOKABLE void addProperty(const QString &name);

    inline Type type() const { return m_type; }
    inline void setType(Type t) { m_type = t; }

    inline LayoutAttached *layout() const { return m_layout; }
    inline void setLayout(LayoutAttached *l) { m_layout = l; }

    inline QString sortProperty() const { return m_sortProperty; }
    inline void setSortProperty(const QString &p) { m_sortProperty = p; }

    static Element *create(QQmlEngine *engine, const QString &name, Element *parent, int id = -1);

    Q_INVOKABLE void publish();

signals:
    void newElementAdded(Element *element, float x, float y);
    void newElementEntered(Element *element, float x, float y);
    void newElementMoved(Element *element, float x, float y);
    void newElementExited(Element *element);

protected:
    void setId(int id);

private slots:
    void focus(wl_surface *surface, int x, int y);
    void motion(uint32_t time, int x, int y);
    void button(uint32_t time, uint32_t button, uint32_t state);

private:
    void setParentElement(Element *parent);
    void sortChildren();

    QString m_typeName;
    Type m_type;
    int m_id;
    Element *m_parent;
    QList<Element *> m_children;
    QStringList m_properties;
    LayoutAttached *m_layout;
    QString m_sortProperty;

    Element *m_target;
    QPointF m_pos;

    static int s_id;

    friend class ShellUI;
};

#endif
