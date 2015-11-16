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
#include "statusnotifiericonprovider.h"
#include "client.h"

void StatusNotifierPlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<StatusNotifierManager>(uri, 1, 0, "StatusNotifierManager", [](QQmlEngine *e, QJSEngine *) {
        StatusNotifierManager *mgr = new StatusNotifierManager;
        e->addImageProvider(QStringLiteral("statusnotifier"), new StatusNotifierIconProvider(mgr));
        return static_cast<QObject *>(mgr);
    });
    qmlRegisterUncreatableType<StatusNotifierItem>(uri, 1, 0, "StatusNotifierItem", QStringLiteral("Cannot create StatusNotifierItem"));
}


StatusNotifierManager::StatusNotifierManager(QObject *p)
                     : QObject(p)
{
    m_watcher = new StatusNotifierWatcher(this);
    if (QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.StatusNotifierWatcher"))) {
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/StatusNotifierWatcher"), this);
        connect(m_watcher, &StatusNotifierWatcher::newItem, this, &StatusNotifierManager::newItem);
    }
}

StatusNotifierManager::~StatusNotifierManager()
{

}

static bool checkSanity(const QString &service)
{
    if (service.contains('/')) {
        return false;
    }
    return true;
}

void StatusNotifierManager::newItem(const QString &service)
{
    if (!checkSanity(service)) {
        return;
    }

    StatusNotifierItem *item = new StatusNotifierItem(service, this);
    connect(item, &StatusNotifierItem::removed, this, &StatusNotifierManager::itemRemoved);
    m_items << item;
    emit itemsChanged();
}

StatusNotifierItem *StatusNotifierManager::item(const QString &service) const
{
    foreach (StatusNotifierItem *item, m_items) {
        if (item->service() == service) {
            return item;
        }
    }
    return nullptr;
}

int StatusNotifierManager::itemsCount(QQmlListProperty<StatusNotifierItem> *prop)
{
    StatusNotifierManager *c = static_cast<StatusNotifierManager *>(prop->object);
    return c->m_items.count();
}

StatusNotifierItem *StatusNotifierManager::itemAt(QQmlListProperty<StatusNotifierItem> *prop, int index)
{
    StatusNotifierManager *c = static_cast<StatusNotifierManager *>(prop->object);
    return c->m_items.at(index);
}

QQmlListProperty<StatusNotifierItem> StatusNotifierManager::items()
{
    return QQmlListProperty<StatusNotifierItem>(this, 0, itemsCount, itemAt);
}

void StatusNotifierManager::itemRemoved()
{
    StatusNotifierItem *item = static_cast<StatusNotifierItem *>(sender());
    m_items.removeOne(item);
    emit itemsChanged();
    delete item;
}

