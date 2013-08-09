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
#include <QGuiApplication>
#include <QQuickWindow>
#include <qpa/qplatformnativeinterface.h>

#include "element.h"
#include "client.h"
#include "grab.h"

static const int a = qmlRegisterType<Element>("Orbital", 1, 0, "Element");
static const int b = qmlRegisterType<ElementConfig>("Orbital", 1, 0, "ElementConfig");

int Element::s_id = 0;

Element::Element(Element *parent)
       : QQuickItem(parent)
       , m_type(Item)
       , m_parent(nullptr)
       , m_layout(nullptr)
       , m_configureItem(nullptr)
       , m_childrenConfig(nullptr)
{
    setParentElement(parent);
}

Element::~Element()
{
    if (m_parent) {
        m_parent->m_children.removeOne(this);
    }
    for (Element *elm: m_children) {
        elm->m_parent = nullptr;
    }
}

void Element::setId(int id)
{
    m_id = id;
    if (id > s_id) {
        s_id = id + 1;
    }
}

void Element::addProperty(const QString &name)
{
    m_properties << name;
}

void Element::publish(const QPointF &offset)
{
    m_target = nullptr;
    m_offset = offset;
    Grab *grab = Client::createGrab();
    connect(grab, SIGNAL(focus(wl_surface *, int, int)), this, SLOT(focus(wl_surface *, int, int)));
    connect(grab, &Grab::motion, this, &Element::motion);
    connect(grab, &Grab::button, this, &Element::button);
    m_properties.clear();
}

void Element::focus(wl_surface *surface, int x, int y)
{
    if (m_target) {
        emit m_target->elementExited(this, m_pos, m_offset);
        m_target->window()->unsetCursor();
    }

    QQuickWindow *window = Client::client()->findWindow(surface);
    QQuickItem *item = window->contentItem()->childAt(x, y);
    m_target = nullptr;
    while (!m_target && item) {
        m_target = qobject_cast<Element *>(item);
        item = item->parentItem();
    }
    window->setCursor(QCursor(Qt::DragMoveCursor));

    m_pos = QPointF(x, y);
    if (m_target) {
        emit m_target->elementEntered(this, m_pos, m_offset);
    }
}

void Element::motion(uint32_t time, int x, int y)
{
    m_pos = QPointF(x, y);

    if (m_target) {
        emit m_target->elementMoved(this, m_pos, m_offset);
    }
}

void Element::button(uint32_t time, uint32_t button, uint32_t state)
{
    if (m_target) {
        setParentElement(m_target);
        m_target->createConfig(this);
        emit m_target->elementAdded(this, m_pos, m_offset);
    } else {
        delete this;
    }
    m_target->window()->unsetCursor();

    static_cast<Grab *>(sender())->end();
}

void Element::setParentElement(Element *parent)
{
    if (parent == m_parent) {
        return;
    }

    if (m_parent) {
        m_parent->m_children.removeOne(this);
    }

    QQuickItem *content = (parent ? parent->property("content").value<QQuickItem *>() : nullptr);
    if (!content) {
        content = parent;
    }
    setParentItem(content);
    if (parent) {
        parent->m_children << this;
    }
    m_parent = parent;
}

void Element::sortChildren()
{
    if (m_sortProperty.isNull()) {
        return;
    }

    struct Sorter {
         QString property;

        bool operator()(Element *a, Element *b) {
            return QQmlProperty::read(a, property).toInt() < QQmlProperty::read(b, property).toInt();
        }
    };

    Sorter sorter = { m_sortProperty };
    qSort(m_children.begin(), m_children.end(), sorter);
}

void Element::createConfig(Element *child)
{
    if (child->m_configureItem) {
        delete child->m_configureItem;
    }
    if (m_childrenConfig) {
        QObject *obj = m_childrenConfig->beginCreate(Client::qmlEngine()->rootContext());
        if (ElementConfig *e = qobject_cast<ElementConfig *>(obj)) {
            e->setParentItem(child);
            e->m_element = child;
            child->m_configureItem = e;
            m_childrenConfig->completeCreate();
        } else {
            qWarning("childrenConfig must be a ElementConfig!");
            delete obj;
        }
    }
}

Element *Element::create(QQmlEngine *engine, const QString &name, int id)
{
    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));
    QQmlComponent c(engine);
    c.loadUrl(path + name + ".qml");
    if (!c.isReady())
        qFatal(qPrintable(c.errorString()));

    QObject *obj = c.create();
    Element *elm = qobject_cast<Element *>(obj);
    if (!elm) {
        qDebug()<<name<<"not an element type.";
        return nullptr;
    }

    if (id < 0) {
        elm->m_id = s_id++;
    } else {
        elm->setId(id);
    }
    elm->m_typeName = name;

    return elm;
}


ElementConfig::ElementConfig(QQuickItem *parent)
             : QQuickItem(parent)
             , m_element(nullptr)
{
}
