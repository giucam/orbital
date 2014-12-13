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

#include <functional>

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusMetaType>
#include <QPoint>
#include <QDebug>

#include "statusnotifieritem.h"

static const QString path = QStringLiteral("/StatusNotifierItem");
static const QString interface = QStringLiteral("org.kde.StatusNotifierItem");

Q_DECLARE_METATYPE(DBusToolTipStruct)
Q_DECLARE_METATYPE(DBusImageStruct)

QDBusArgument &operator<<(QDBusArgument &argument, const DBusImageStruct &v)
{
    argument.beginStructure();
    argument << v.width << v.height << v.data;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusImageStruct &v)
{
    argument.beginStructure();
    argument >> v.width >> v.height >> v.data;
    argument.endStructure();
    return argument;
}


QDBusArgument &operator<<(QDBusArgument &argument, const DBusToolTipStruct &v)
{
    argument.beginStructure();
    argument << v.icon << v.image << v.title << v.subTitle;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusToolTipStruct &v)
{
    argument.beginStructure();
    argument >> v.icon >> v.image >> v.title >> v.subTitle;
    argument.endStructure();
    return argument;
}

static void getProperty(const QString service, const QString property, std::function<void (const QVariant &)> func)
{
    QDBusInterface iface(service, path, QStringLiteral("org.freedesktop.DBus.Properties"), QDBusConnection::sessionBus());

    QDBusPendingCall call = iface.asyncCall("Get", interface, property);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [func, property](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariant> reply = *watcher;
        if (reply.isError()) {
            qDebug() << "Error retrieving" << property << reply.error().message();
        } else {
            func(reply.value());
        }
    });
}

StatusNotifierItem::StatusNotifierItem(const QString &service, QObject *p)
                  : QObject(p)
                  , m_service(service)
                  , m_interface(m_service, path, interface, QDBusConnection::sessionBus())
{
    qDBusRegisterMetaType<DBusImageStruct>();
    qDBusRegisterMetaType<DBusToolTipStruct>();

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(service, bus, QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &StatusNotifierItem::removed);
    bus.connect(service, path, interface, QStringLiteral("NewTitle"), this, SLOT(getTitle()));
    bus.connect(service, path, interface, QStringLiteral("NewIcon"), this, SLOT(getIcon()));
    bus.connect(service, path, interface, QStringLiteral("NewToolTip"), this, SLOT(getTooltip()));

    getProperty(m_service, QStringLiteral("Id"), [this](const QVariant &v) {
        m_name = v.toString();
        emit nameChanged();
    });
    getIcon();
    getTitle();
    getTooltip();
}

StatusNotifierItem::~StatusNotifierItem()
{
}

QString StatusNotifierItem::name() const
{
    return m_name;
}

QString StatusNotifierItem::title() const
{
    return m_title;
}

QString StatusNotifierItem::iconName() const
{
    return m_iconName;
}

QString StatusNotifierItem::tooltipTitle() const
{
    return m_tooltip.title;
}

void StatusNotifierItem::activate()
{
    m_interface.asyncCall(QStringLiteral("Activate"), 0, 0);
}

void StatusNotifierItem::secondaryActivate()
{
    m_interface.asyncCall(QStringLiteral("SecondaryActivate"), 0, 0);
}

void StatusNotifierItem::scroll(const QPoint &delta)
{
    QString method = QStringLiteral("Scroll");
    if (delta.y() != 0) {
        m_interface.asyncCall(method, delta.y(), "vertical");
    }
    if (delta.x() != 0) {
        m_interface.asyncCall(method, delta.x(), "horizontal");
    }
}

void StatusNotifierItem::getTitle()
{
    getProperty(m_service, QStringLiteral("Title"), [this](const QVariant &v) {
        m_title = v.toString();
        emit titleChanged();
    });
}

void StatusNotifierItem::getIcon()
{
    getProperty(m_service, QStringLiteral("IconName"), [this](const QVariant &v) {
        m_iconName = v.toString();
        emit iconNameChanged();
    });
}

void StatusNotifierItem::getTooltip()
{
    getProperty(m_service, QStringLiteral("ToolTip"), [this](const QVariant &v) {
        v.value<QDBusArgument>() >> m_tooltip;
        emit tooltipChanged();
    });
}