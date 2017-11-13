/*
 * Copyright 2015 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <linux/input.h>

#include <QDebug>
#include <QtQml>
#include <qmath.h>
#include <QDBusReply>
#include <QDBusConnectionInterface>

#include "mprisservice.h"
#include "dbusinterface.h"

#define DBUS_SERVICE QStringLiteral("org.freedesktop.DBus")
#define MPRIS_PATH QStringLiteral("/org/mpris/MediaPlayer2")
#define MPRIS_INTERFACE QStringLiteral("org.mpris.MediaPlayer2")
#define MPRIS_PLAYER_INTERFACE QStringLiteral("org.mpris.MediaPlayer2.Player")
#define DBUS_PROPERTIES_INTERFACE QStringLiteral("org.freedesktop.DBus.Properties")

void MprisPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<Mpris>(uri, 1, 0, "Mpris");
}

Mpris::Mpris(QObject *p)
            : QObject(p)
            , m_valid(false)
            , m_pid(0)
            , m_playbackStatus(PlaybackStatus::Stopped)
            , m_trackLength(0)
            , m_trackPosition(0)
            , m_rate(1)
{
    m_posTimer.setInterval(1000);
    connect(&m_posTimer, &QTimer::timeout, this, &Mpris::updatePos);
}

Mpris::~Mpris()
{
}

bool Mpris::isValid() const
{
    return m_valid;
}

quint64 Mpris::pid() const
{
    return m_pid;
}

void Mpris::setPid(quint64 pid)
{
    if (m_pid != pid) {
        m_pid = pid;
        emit targetChanged();
        setValid(false);
        checkConnection();
    }
}

#define CALL(method) \
    DBusInterface iface(m_service, MPRIS_PATH, MPRIS_PLAYER_INTERFACE); \
    iface.asyncCall(QStringLiteral(method))

void Mpris::playPause()
{
    CALL("PlayPause");
}

void Mpris::stop()
{
    CALL("Stop");
}

void Mpris::previous()
{
    CALL("Previous");
}

void Mpris::next()
{
    CALL("Next");
}

void Mpris::checkConnection()
{
    if (m_valid) {
        QDBusConnection::sessionBus().disconnect(m_service, MPRIS_PATH, DBUS_PROPERTIES_INTERFACE,
                                                 QStringLiteral("PropertiesChanged"),
                                                 this, SLOT(propertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
    }

    DBusInterface iface(DBUS_SERVICE, QStringLiteral("/"), DBUS_SERVICE);
    QDBusPendingCall call = iface.asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QStringList> reply = *watcher;

        if (reply.isError()) {
            qDebug("Failed to get the list of DBus services.");
        } else {
            QStringList services = reply.value();
            checkServices(services);
        }
    });
}

void Mpris::checkServices(const QStringList &services)
{
    DBusInterface iface(DBUS_SERVICE, QStringLiteral("/"), DBUS_SERVICE);
    foreach (const QString &service, services) {
        QDBusPendingCall call = iface.asyncCall(QStringLiteral("GetConnectionUnixProcessID"), service);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
        watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [this, service](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            QDBusPendingReply<quint32> reply = *watcher;

            if (reply.isError()) {
                qDebug("Unable to get the pid of service %s.", qPrintable(service));
            } else {
                quint32 p = reply.value();
                if (p == m_pid) {
                    checkService(service);
                }
            }
        });
    }
}

static QDBusPendingCall getProperty(const QString &service, const QString &path,
                                    const QString &interface, const QString &property)
{
    DBusInterface iface(service, path, DBUS_PROPERTIES_INTERFACE);
    return iface.asyncCall(QStringLiteral("Get"), interface, property);
}

void Mpris::checkService(const QString &service)
{
    QDBusPendingCall call = ::getProperty(service, MPRIS_PATH, MPRIS_INTERFACE, QStringLiteral("Identity"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [this, service](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariant> reply = *watcher;
        if (m_valid) {
            return;
        }

        if (reply.isError()) {
            qDebug("%s is not a valid mpris service.", qPrintable(service));
        } else {
            setValid(true);
            m_service = service;
            qDebug("Found %s at %s.", qPrintable(reply.value().toString()), qPrintable(service));

            QDBusConnection::sessionBus().connect(service, MPRIS_PATH, DBUS_PROPERTIES_INTERFACE,
                                                QStringLiteral("PropertiesChanged"),
                                                this, SLOT(propertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
            QDBusConnection::sessionBus().connect(service, MPRIS_PATH, MPRIS_PLAYER_INTERFACE,
                                                  QStringLiteral("Seeked"),
                                                  this, SLOT(seeked(qint64)));
            getMetadata();
            getPosition();
            getPlaybackStatus();
            getRate();
        }
    });
}

inline void Mpris::setValid(bool v)
{
    if (m_valid != v) {
        m_valid = v;
        emit validChanged();
    }
}

void Mpris::getProperty(const QString &property, const std::function<void (const QVariant &v)> &func)
{
    QDBusPendingCall call = ::getProperty(m_service, MPRIS_PATH, MPRIS_PLAYER_INTERFACE, property);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [property, func](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariant> reply = *watcher;

        if (reply.isError()) {
            qDebug("Mpris: failed to retrieve property '%s'.", qPrintable(property));
            func(QVariant());
        } else {
            func(reply.value());
        }
    });
}

void Mpris::getMetadata()
{
    getProperty(QStringLiteral("Metadata"), [this](const QVariant &v) {
        QVariantMap md;
        v.value<QDBusArgument>() >> md;
        updateMetadata(md);
    });
}

void Mpris::updateMetadata(const QVariantMap &md)
{
    m_trackTitle = QString();
    m_trackLength = 0;
    m_trackPosition = 0;
    for (auto i = md.begin(); i != md.end(); ++i) {
        if (i.key() == QStringLiteral("xesam:title")) {
            m_trackTitle = i.value().toString();
        } else if (i.key() == QStringLiteral("mpris:length")) {
            m_trackLength = i.value().toLongLong() / 1000;
        }
    }
    emit trackTitleChanged();
    emit trackLengthChanged();
}

void Mpris::getPlaybackStatus()
{
    getProperty(QStringLiteral("PlaybackStatus"), [this](const QVariant &v) {
        updatePlaybackStatus(v.toString());
    });
}

void Mpris::updatePlaybackStatus(const QString &st)
{
    PlaybackStatus old = m_playbackStatus;

    m_posTimer.stop();
    if (st == QStringLiteral("Playing")) {
        m_playbackStatus = PlaybackStatus::Playing;
        m_posTimer.start();
    } else if (st == QStringLiteral("Paused")) {
        if (m_playbackStatus == PlaybackStatus::Playing) {
            m_trackPosition += m_posTimer.remainingTime();
            emit trackPositionChanged();
        }
        m_playbackStatus = PlaybackStatus::Paused;
    } else {
        m_playbackStatus = PlaybackStatus::Stopped;
        m_trackPosition = 0;
        emit trackPositionChanged();
    }
    if (m_playbackStatus != old) {
        emit playbackStatusChanged();
    }
}

void Mpris::getRate()
{
    getProperty(QStringLiteral("Rate"), [this](const QVariant &v) {
        updateRate(v.toDouble());
    });
}

void Mpris::updateRate(double rate)
{
    m_rate = rate;
    emit rateChanged();
}

void Mpris::getPosition()
{
    getProperty(QStringLiteral("Position"), [this](const QVariant &v) {
        m_trackPosition = v.toDouble() / 1000;
        emit trackPositionChanged();
    });
}

void Mpris::updatePos()
{
    m_trackPosition += m_posTimer.interval();
    emit trackPositionChanged();
}

quint32 Mpris::trackPosition() const
{
    // https://github.com/clementine-player/Clementine/issues/5097
    if (m_trackPosition > m_trackLength) {
        return 0;
    }
    return m_trackPosition;
}

void Mpris::propertiesChanged(const QString &, const QMap<QString, QVariant> &changed, const QStringList &invalidated)
{
    static const QString metadata = QStringLiteral("Metadata");
    if (changed.contains(metadata)) {
        QVariantMap md;
        changed.value(metadata).value<QDBusArgument>() >> md;
        updateMetadata(md);
    } else if (invalidated.contains(metadata)) {
        getMetadata();
    }
    static const QString playbackStatus = QStringLiteral("PlaybackStatus");
    if (changed.contains(playbackStatus)) {
        updatePlaybackStatus(changed.value(playbackStatus).toString());
    } else if (invalidated.contains(playbackStatus)) {
        getPlaybackStatus();
    }

    static const QString rate = QStringLiteral("Rate");
    if (changed.contains(rate)) {
        updateRate(changed.value(rate).toDouble());
    } else if (invalidated.contains(rate)) {
        getRate();
    }
}

void Mpris::seeked(qint64 time)
{
    m_trackPosition = time / 1000;
    emit trackPositionChanged();
}
