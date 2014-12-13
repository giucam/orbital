/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <QDBusConnection>
#include <QtQml>
#include <QDebug>

#include "statusnotifierservice.h"
#include "statusnotifierwatcher.h"
#include "statusnotifieritem.h"

StatusNotifierService::StatusNotifierService()
                     : Service()
{
    qmlRegisterUncreatableType<StatusNotifierItem>("Orbital", 1, 0, "StatusNotifierItem", "Cannot create StatusNotifierItem");
}

StatusNotifierService::~StatusNotifierService()
{

}

void StatusNotifierService::init()
{
    m_watcher = new StatusNotifierWatcher(this);
    if (QDBusConnection::sessionBus().registerService("org.kde.StatusNotifierWatcher")) {
        QDBusConnection::sessionBus().registerObject("/StatusNotifierWatcher", this);
        connect(m_watcher, &StatusNotifierWatcher::newItem, this, &StatusNotifierService::newItem);
    }
}

void StatusNotifierService::newItem(const QString &service)
{
    StatusNotifierItem *item = new StatusNotifierItem(service, this);
    connect(item, &StatusNotifierItem::removed, this, &StatusNotifierService::itemRemoved);
    m_items << item;
    emit itemsChanged();
}

int StatusNotifierService::itemsCount(QQmlListProperty<StatusNotifierItem> *prop)
{
    StatusNotifierService *c = static_cast<StatusNotifierService *>(prop->object);
    return c->m_items.count();
}

StatusNotifierItem *StatusNotifierService::itemAt(QQmlListProperty<StatusNotifierItem> *prop, int index)
{
    StatusNotifierService *c = static_cast<StatusNotifierService *>(prop->object);
    return c->m_items.at(index);
}

QQmlListProperty<StatusNotifierItem> StatusNotifierService::items()
{
    return QQmlListProperty<StatusNotifierItem>(this, 0, itemsCount, itemAt);
}

void StatusNotifierService::itemRemoved()
{
    StatusNotifierItem *item = static_cast<StatusNotifierItem *>(sender());
    m_items.removeOne(item);
    emit itemsChanged();
    delete item;
}

