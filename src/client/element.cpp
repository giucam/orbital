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

#include <QtQml>

#include "element.h"

static const int a = qmlRegisterType<Element>("Orbital", 1, 0, "Element");

int Element::s_id = 0;

Element::Element(Element *parent)
       : QQuickItem(parent)
       , m_type(Item)
       , m_id(s_id++)
{
    setParentElement(parent);
}

void Element::setId(int id)
{
    m_id = id;
    if (id > s_id) {
        s_id = id;
    }
}

void Element::addProperty(const QString &name)
{
    m_properties << name;
}

void Element::setParentElement(Element *parent)
{
    if (!parent) {
        return;
    }

    QQuickItem *content = parent->property("content").value<QQuickItem *>();
    if (!content) {
        content = parent;
    }
    setParentItem(content);
    parent->m_children << this;
}

Element *Element::create(QQmlEngine *engine, const QString &name, Element *parent)
{
    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));
    QQmlComponent *c = new QQmlComponent(engine, 0);
    c->loadUrl(path + name + ".qml");
    if (!c->isReady())
        qFatal(qPrintable(c->errorString()));

    QObject *obj = c->create();
    Element *elm = qobject_cast<Element *>(obj);
    if (!elm) {
        qDebug()<<name<<"not an element type.";
        return nullptr;
    }

    elm->setParentElement(parent);
    elm->m_id = s_id++;
    elm->m_typeName = name;

    return elm;
}
