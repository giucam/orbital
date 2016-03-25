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

#define PATH QStringLiteral("/StatusNotifierItem")
#define INTERFACE QStringLiteral("org.kde.StatusNotifierItem")

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

static void getProperty(const QString service, const QString property, std::function<void (const QVariant &)> func, const std::function<void ()> &errorFunc = nullptr)
{
    DBusInterface iface(service, PATH, QStringLiteral("org.freedesktop.DBus.Properties"), QDBusConnection::sessionBus());

    QDBusPendingCall call = iface.asyncCall(QStringLiteral("Get"), INTERFACE, property);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [func, property, errorFunc](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariant> reply = *watcher;
        if (reply.isError()) {
            qDebug() << "Error retrieving" << property << reply.error().message();
            if (errorFunc) {
                errorFunc();
            }
        } else {
            func(reply.value());
        }
    });
}

StatusNotifierItem::StatusNotifierItem(const QString &service, QObject *p)
                  : QObject(p)
                  , m_service(service)
                  , m_interface(m_service, PATH, INTERFACE, QDBusConnection::sessionBus())
{
    qDBusRegisterMetaType<DBusImageStruct>();
    qDBusRegisterMetaType<DBusToolTipStruct>();

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(service, bus, QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &StatusNotifierItem::removed);
    bus.connect(service, PATH, INTERFACE, QStringLiteral("NewTitle"), this, SLOT(getTitle()));
    bus.connect(service, PATH, INTERFACE, QStringLiteral("NewIcon"), this, SLOT(getIcon()));
    bus.connect(service, PATH, INTERFACE, QStringLiteral("NewAttentionIcon"), this, SLOT(getAttentionIcon()));
    bus.connect(service, PATH, INTERFACE, QStringLiteral("NewToolTip"), this, SLOT(getTooltip()));
    bus.connect(service, PATH, INTERFACE, QStringLiteral("NewStatus"), this, SLOT(getStatus()));

    m_icon.updatePending = 0;
    m_attentionIcon.updatePending = 0;

    getProperty(m_service, QStringLiteral("Id"), [this](const QVariant &v) {
        m_name = v.toString();
        emit nameChanged();
    });
    getIcon();
    getTitle();
    getTooltip();
    getStatus();
    getAttentionIcon();
}

StatusNotifierItem::~StatusNotifierItem()
{
}

QString StatusNotifierItem::service() const
{
    return m_service;
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
    return m_icon.name;
}

static QPixmap pixmap(const QSize &s, const DBusImageVector &vec)
{
    const DBusImageStruct *image = nullptr;
    int dw, dh;
    foreach (const DBusImageStruct &img, vec) {
        int _dw = qAbs(img.width - s.width());
        int _dh = qAbs(img.height - s.height());
        if (!image || _dw < dw || _dh < dh) {
            image = &img;
            dw = _dw;
            dh = _dh;
        }
    }
    if (image) {
        QImage img((const uchar *)image->data.constData(), image->width, image->height, QImage::Format_ARGB32);
        return QPixmap::fromImage(img);
    }
    return QPixmap();
}

QPixmap StatusNotifierItem::iconPixmap(const QSize &s) const
{
    return pixmap(s, m_icon.pixmap);
}

QString StatusNotifierItem::attentionIconName() const
{
    return m_attentionIcon.name;
}

QPixmap StatusNotifierItem::attentionIconPixmap(const QSize &s) const
{
    return pixmap(s, m_attentionIcon.pixmap);
}

QString StatusNotifierItem::tooltipTitle() const
{
    return m_tooltip.title;
}

StatusNotifierItem::Status StatusNotifierItem::status() const
{
    return m_status;
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
        m_interface.asyncCall(method, delta.y(), QLatin1String("vertical"));
    }
    if (delta.x() != 0) {
        m_interface.asyncCall(method, delta.x(), QLatin1String("horizontal"));
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
    auto decreaseUpdatePending = [this]() {
        if (--m_icon.updatePending == 0)
            emit iconChanged();
    };

    m_icon.updatePending += 2;
    getProperty(m_service, QStringLiteral("IconName"), [=](const QVariant &v) {
        m_icon.name = v.toString();
        decreaseUpdatePending();
    }, decreaseUpdatePending);
    getProperty(m_service, QStringLiteral("IconPixmap"), [=](const QVariant &v) {
        v.value<QDBusArgument>() >> m_icon.pixmap;
        decreaseUpdatePending();
    }, decreaseUpdatePending);
}

void StatusNotifierItem::getAttentionIcon()
{
    auto decreaseUpdatePending = [this]() {
        if (--m_attentionIcon.updatePending == 0)
            emit attentionIconChanged();
    };

    m_attentionIcon.updatePending += 2;
    getProperty(m_service, QStringLiteral("AttentionIconName"), [=](const QVariant &v) {
        m_attentionIcon.name = v.toString();
        if (--m_attentionIcon.updatePending)
            emit attentionIconChanged();
    }, decreaseUpdatePending);
    getProperty(m_service, QStringLiteral("AttentionIconPixmap"), [=](const QVariant &v) {
        v.value<QDBusArgument>() >> m_attentionIcon.pixmap;
        if (--m_attentionIcon.updatePending)
            emit attentionIconChanged();
    }, decreaseUpdatePending);
}

void StatusNotifierItem::getTooltip()
{
    getProperty(m_service, QStringLiteral("ToolTip"), [this](const QVariant &v) {
        v.value<QDBusArgument>() >> m_tooltip;
        emit tooltipChanged();
    });
}

void StatusNotifierItem::getStatus()
{
    getProperty(m_service, QStringLiteral("Status"), [this](const QVariant &v) {
        QString str = v.toString();
        if (str == QStringLiteral("NeedsAttention")) {
            m_status = Status::NeedsAttention;
        } else if (str == QStringLiteral("Active")) {
            m_status = Status::Active;
        } else {
            m_status = Status::Passive;
        }
        emit statusChanged();
    });
}
