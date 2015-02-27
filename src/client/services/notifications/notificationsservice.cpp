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

#include <QtQml>
#include <QDBusConnection>
#include <QDebug>

#include "client.h"
#include "notificationsservice.h"
#include "notificationsadaptor.h"
#include "notificationsiconprovider.h"

void NotificationsPlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<NotificationsManager>(uri, 1, 0, "NotificationsManager", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(new NotificationsManager);
    });
    qmlRegisterUncreatableType<Notification>(uri, 1, 0, "Notification", "Cannot create Notification objects");
}


Notification::Notification()
            : QObject()
{
}

void Notification::timerEvent(QTimerEvent *e)
{
    emit expired();
    deleteLater();
}

void Notification::setId(int id)
{
    m_id = id;
}

void Notification::setSummary(const QString &s)
{
    m_summary = s;
}

void Notification::setBody(const QString &b)
{
    m_body = b;
}

void Notification::setIconName(const QString &n)
{
    m_iconName = n;
}

void Notification::setIconImage(const QPixmap &img)
{
    m_iconImage = img;
}



NotificationsManager::NotificationsManager(QObject *p)
                    : QObject(p)
{
    QStringList caps = { "actions", "action-icons", "body-markup" };
    new NotificationsAdaptor(this, caps);
    if (QDBusConnection::sessionBus().registerService("org.freedesktop.Notifications")) {
        QDBusConnection::sessionBus().registerObject("/org/freedesktop/Notifications", this);
    }

    Client::client()->qmlEngine()->addImageProvider(QStringLiteral("notifications"), new NotificationsIconProvider(this));
}

NotificationsManager::~NotificationsManager()
{

}

void NotificationsManager::newNotification(Notification *n)
{
    int id = n->id();
    m_notifications[id] = n;
    n->startTimer(5000);
    connect(n, &Notification::expired, [this, id]() { m_notifications.remove(id); });
    emit notify(n);
}

Notification *NotificationsManager::notification(int id) const
{
    return m_notifications.value(id);
}
