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

#ifndef STATUSNOTIFIERSERVICE_H
#define STATUSNOTIFIERSERVICE_H

#include <QQmlListProperty>

#include "service.h"

class StatusNotifierWatcher;
class StatusNotifierItem;

class StatusNotifierService : public Service
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<StatusNotifierItem> items READ items NOTIFY itemsChanged)
    Q_INTERFACES(Service)
    Q_PLUGIN_METADATA(IID "Orbital.Service")
public:
    StatusNotifierService();
    ~StatusNotifierService();

    void init() override;

    QQmlListProperty<StatusNotifierItem> items();

signals:
    void itemsChanged();

private:
    void newItem(const QString &service);
    void itemRemoved();
    static int itemsCount(QQmlListProperty<StatusNotifierItem> *prop);
    static StatusNotifierItem *itemAt(QQmlListProperty<StatusNotifierItem> *prop, int index);

    StatusNotifierWatcher *m_watcher;
    QList<StatusNotifierItem *> m_items;
};

#endif
