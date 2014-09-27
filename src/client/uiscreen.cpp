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
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QJsonArray>

#include "client.h"
#include "element.h"
#include "shellui.h"
#include "panel.h"

UiScreen::UiScreen(ShellUI *ui, Client *client, QScreen *screen, const QString &name)
       : QObject(client)
       , m_client(client)
       , m_ui(ui)
       , m_name(name)
       , m_screen(screen)
       , m_loading(true)
{
}

UiScreen::~UiScreen()
{
    qDeleteAll(m_children);
}

void UiScreen::loadConfig(QJsonObject &config)
{
    for (Element *elm: m_elements) {
        if (elm->m_parent) {
            elm->setParentElement(nullptr);
        }
    }

    QHash<int, Element *> oldElements = m_elements;
    m_elements.clear();

    QJsonArray elements = config["elements"].toArray();
    for (auto i = elements.begin(); i != elements.end(); ++i) {
        QJsonObject element = (*i).toObject();
        Element *elm = loadElement(nullptr, element, &oldElements);
        if (elm) {
            elm->setParent(this);
            m_children << elm;
        }
        *i = element;
    }
    config["elements"] = elements;

    for (Element *e: oldElements) {
        e->deleteLater();
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

            connect(m_screen, &QObject::destroyed, [window]() { delete window; });
            connect(elm, &QObject::destroyed, [window]() { delete window; });
            window->setScreen(m_screen);
            window->setWidth(elm->width());
            window->setHeight(elm->height());
            window->setColor(Qt::transparent);
            window->setFlags(Qt::BypassWindowManagerHint);
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

    // wait until all the objects have finished what they're doing before sending the loaded event
    QTimer::singleShot(0, this, SLOT(screenLoaded()));
}

Element *UiScreen::loadElement(Element *parent, QJsonObject &config, QHash<int, Element *> *elements)
{
    if (!config.keys().contains("type")) {
        return nullptr;
    }

    bool created = false;
    int id = config.contains("id") ? config["id"].toInt() : -1;
    Element *elm = (elements ? elements->take(id) : nullptr);
    if (!elm) {
        QString type = config["type"].toString();
        elm = Element::create(m_ui, this, m_ui->qmlEngine(), type, id);
        if (!elm) {
            return nullptr;
        }
        connect(elm, &QObject::destroyed, this, &UiScreen::elementDestroyed);
        created = true;
    }
    if (parent) {
        elm->setParentElement(parent);
    }
    elm->m_properties.clear();
    m_elements.insert(elm->m_id, elm);
    config["id"] = elm->m_id;

    QJsonObject properties = config["properties"].toObject();
    for (auto i = properties.constBegin(); i != properties.constEnd(); ++i) {
        QString name = i.key();
        QVariant value = i.value().toVariant();

        if (name == "location") {
            elm->setLocation((Element::Location)value.toInt());
        } else {
            QQmlProperty::write(elm, name, value);
        }
        elm->addProperty(name);
    }

    QJsonArray children = config["elements"].toArray();
    for (auto i = children.begin(); i != children.end(); ++i) {
        QJsonObject cfg = (*i).toObject();
        loadElement(elm, cfg, elements);
        *i = cfg;
    }
    config["elements"] = children;

    if (created && parent) {
        parent->createConfig(elm);
        parent->createBackground(elm);
    }
    return (created ? elm : nullptr);
}

void UiScreen::saveConfig(QJsonObject &config)
{
    saveChildren(m_children, config);
}

void UiScreen::saveProperties(QObject *obj, const QStringList &properties, QJsonObject &config)
{
    if (properties.isEmpty()) {
        return;
    }
    QJsonObject cfg = config["properties"].toObject();
    for (const QString &prop: properties) {
        QVariant value = obj->property(qPrintable(prop));
        bool ok;
        int v = value.toInt(&ok);
        if (ok) {
            cfg[prop] = v;
        } else {
            cfg[prop] = value.toString();
        }
    }
    config["properties"] = cfg;
}

void UiScreen::saveChildren(const QList<Element *> &children, QJsonObject &config)
{
    if (children.isEmpty()) {
        return;
    }

    QJsonArray elements;
    for (Element *child: children) {
        QJsonObject cfg;
        cfg["type"] = child->m_typeName;
//         cfg["id"] = child->m_id;

        saveProperties(child, child->m_ownProperties, cfg);
        saveProperties(child, child->m_properties, cfg);
        child->sortChildren();
        saveChildren(child->m_children, cfg);

        elements << cfg;
    }
    config["elements"] = elements;
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

bool UiScreen::loading() const
{
    return m_loading;
}

void UiScreen::screenLoaded()
{
    m_loading = false;
    emit loaded();
}
