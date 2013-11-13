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

#include "uiscreen.h"

#include <QIcon>
#include <QDebug>
#include <QQuickItem>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickWindow>

#include "client.h"
#include "element.h"
#include "shellui.h"
#include "panel.h"

UiScreen::UiScreen(ShellUI *ui, Client *client, int id, QScreen *screen)
       : QObject(client)
       , m_client(client)
       , m_ui(ui)
       , m_id(id)
       , m_screen(screen)
{
}

UiScreen::~UiScreen()
{
    qDeleteAll(m_children);
}

void UiScreen::loadConfig(QXmlStreamReader &xml)
{
    for (Element *elm: m_elements) {
        if (elm->m_parent) {
            elm->setParentElement(nullptr);
        }
    }

    QHash<int, Element *> oldElements = m_elements;
    m_elements.clear();

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                Element *elm = loadElement(nullptr, xml, &oldElements);
                if (elm) {
                    elm->setParent(this);
                    m_children << elm;
                }
            }
        } else if (xml.name() == "Screen") {
            break;
        }
    }

    for (Element *e: oldElements) {
        delete e;
        m_children.removeOne(e);
    }

    for (Element *elm: m_children) {
        if (elm->type() == ElementInfo::Type::Item)
            continue;

        if (elm->type() == ElementInfo::Type::Panel) {
            Panel *p = static_cast<Panel *>(elm->window());
            if (!p) {
                p = new Panel(m_screen, elm);
            }
            p->setLocation(elm->location());
        } else {
            QQuickWindow *window = m_client->window(elm);

            window->setWidth(elm->width());
            window->setHeight(elm->height());
            window->setColor(Qt::transparent);
            window->setFlags(Qt::BypassWindowManagerHint);
            window->setScreen(m_screen);
            window->show();
            window->create();

            m_client->setInputRegion(window, elm->inputRegion());

            switch (elm->type()) {
                case ElementInfo::Type::Background:
                    m_client->setBackground(window, m_screen);
                    break;
                case ElementInfo::Type::Panel:
                    m_client->setPanel(window, m_screen, (int)elm->location());
                    break;
                case ElementInfo::Type::Overlay:
                    m_client->addOverlay(window, m_screen);
                    break;
                default:
                    break;
            }
        }
    }

}

Element *UiScreen::loadElement(Element *parent, QXmlStreamReader &xml, QHash<int, Element *> *elements)
{
    QXmlStreamAttributes attribs = xml.attributes();
    if (!attribs.hasAttribute("type")) {
        return nullptr;
    }

    bool created = false;
    int id = attribs.value("id").toInt();
    Element *elm = (elements ? elements->take(id) : nullptr);
    if (!elm) {
        QString type = attribs.value("type").toString();
        elm = Element::create(m_ui, m_ui->qmlEngine(), type, id);
        if (!elm) {
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isEndElement() && xml.name() == "element") {
                    xml.readNext();
                    return nullptr;
                }
            }
            return nullptr;
        }
        connect(elm, &QObject::destroyed, this, &UiScreen::elementDestroyed);
        elm->m_screen = this;
        emit elm->screenChanged();
        created = true;
    }
    if (parent) {
        elm->setParentElement(parent);
    }
    elm->m_properties.clear();
    m_elements.insert(id, elm);

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                loadElement(elm, xml, elements);
            } else if (xml.name() == "property") {
                QXmlStreamAttributes attribs = xml.attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();

                if (name == "location") {
                    elm->setLocation((Element::Location)value.toInt());
                } else {
                    QQmlProperty::write(elm, name, value);
                }
                elm->addProperty(name);
            }
        }
        if (xml.isEndElement() && xml.name() == "element") {
            xml.readNext();
            if (created && parent) {
                parent->createConfig(elm);
                parent->createBackground(elm);
            }
            return (created ? elm : nullptr);
        }
    }
    return (created ? elm : nullptr);
}

void UiScreen::saveConfig(QXmlStreamWriter &xml)
{
    xml.writeStartElement("Screen");
    xml.writeAttribute("output", QString::number(m_id));

    saveChildren(m_children, xml);

    xml.writeEndElement();
}

void UiScreen::saveElement(Element *elm, QXmlStreamWriter &xml)
{
    saveProperties(elm, elm->m_ownProperties, xml);
    saveProperties(elm, elm->m_properties, xml);
    elm->sortChildren();
    saveChildren(elm->m_children, xml);
}

void UiScreen::saveProperties(QObject *obj, const QStringList &properties, QXmlStreamWriter &xml)
{
    for (const QString &prop: properties) {
        xml.writeStartElement("property");
        xml.writeAttribute("name", prop);
        xml.writeAttribute("value", obj->property(qPrintable(prop)).toString());
        xml.writeEndElement();
    }
}

void UiScreen::saveChildren(const QList<Element *> &children, QXmlStreamWriter &xml)
{
    for (Element *child: children) {
        xml.writeStartElement("element");
        xml.writeAttribute("type", child->m_typeName);
        xml.writeAttribute("id", QString::number(child->m_id));

        saveElement(child, xml);

        xml.writeEndElement();
    }

}

void UiScreen::addElement(Element *elm)
{
    if (elm->m_screen == this) {
        return;
    }

    if (elm->m_screen) {
        elm->m_screen->removeElement(elm);
    }

    m_elements.insert(elm->m_id, elm);
    elm->m_screen = this;
    emit elm->screenChanged();
    connect(elm, &QObject::destroyed, this, &UiScreen::elementDestroyed);
}

void UiScreen::removeElement(Element *elm)
{
    if (elm->m_screen != this) {
        return;
    }

    m_children.removeOne(elm);
    m_elements.remove(elm->m_id);
    elm->m_screen = nullptr;
    disconnect(elm, &QObject::destroyed, this, &UiScreen::elementDestroyed);
}

void UiScreen::elementDestroyed(QObject *obj)
{
    Element *elm = static_cast<Element *>(obj);
    m_children.removeOne(elm);
    m_elements.remove(elm->m_id);
}

void UiScreen::setAvailableRect(const QRect &r)
{
    m_rect = r;
    emit availableRectChanged();
}
