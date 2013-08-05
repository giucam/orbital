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

class Element : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Type type READ type WRITE setType)
public:
    enum Type {
        Item,
        Background,
        Panel,
        Overlay
    };
    Q_ENUMS(Type)
    explicit Element(Element *parent = nullptr);

    void addProperty(const QString &name);

    inline Type type() const { return m_type; }
    inline void setType(Type t) { m_type = t; }

    static Element *create(QQmlEngine *engine, const QString &name, Element *parent);

protected:
    void setId(int id);

private:
    void setParentElement(Element *parent);

    QString m_typeName;
    Type m_type;
    int m_id;
    QList<Element *> m_children;
    QStringList m_properties;

    static int s_id;

    friend class ShellUI;
};

#endif
