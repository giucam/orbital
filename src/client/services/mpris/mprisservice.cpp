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
#include <QDBusInterface>

#include "mprisservice.h"

static const QString dbusService = QStringLiteral("org.freedesktop.DBus");
static const QString mprisPath = QStringLiteral("/org/mpris/MediaPlayer2");
static const QString mprisInterface = QStringLiteral("org.mpris.MediaPlayer2");
static const QString mprisPlayerInterface = QStringLiteral("org.mpris.MediaPlayer2.Player");
static const QString dbusPropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");

void MprisPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<Mpris>(uri, 1, 0, "Mpris");
}



Mpris::Mpris(QObject *p)
            : QObject(p)
            , m_pid(0)
            , m_playbackStatus(PlaybackStatus::Stopped)
{
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
    QDBusInterface iface(m_service, mprisPath, mprisPlayerInterface); \
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
        QDBusConnection::sessionBus().disconnect(m_service, mprisPath, dbusPropertiesInterface,
                                                 QStringLiteral("PropertiesChanged"),
                                                 this, SLOT(propertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
    }

    QDBusInterface iface(dbusService, QStringLiteral("/"), dbusService);
    if (!iface.isValid()) {
        return;
    }
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
    QDBusInterface iface(dbusService, QStringLiteral("/"), dbusService);
    foreach (QString service, services) {
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
    QDBusInterface iface(service, path, dbusPropertiesInterface);
    return iface.asyncCall(QStringLiteral("Get"), interface, property);
}

void Mpris::checkService(const QString &service)
{
    QDBusPendingCall call = getProperty(service, mprisPath, mprisInterface, QStringLiteral("Identity"));
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

            QDBusConnection::sessionBus().connect(service, mprisPath, dbusPropertiesInterface,
                                                QStringLiteral("PropertiesChanged"),
                                                this, SLOT(propertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
            getMetadata();
            getPlaybackStatus();
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

void Mpris::getMetadata()
{
    QDBusPendingCall call = getProperty(m_service, mprisPath, mprisPlayerInterface, QStringLiteral("Metadata"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariant> reply = *watcher;

        QVariantMap md;
        if (reply.isError()) {
            qDebug("Failed to retrieve mpris metadata.");
        } else {
            reply.value().value<QDBusArgument>() >> md;
        }
        updateMetadata(md);
    });
}

void Mpris::updateMetadata(const QVariantMap &md)
{
    m_trackTitle = QString();
    for (auto i = md.begin(); i != md.end(); ++i) {
        if (i.key() == QStringLiteral("xesam:title")) {
            m_trackTitle = i.value().toString();
        }
    }
    emit trackTitleChanged();
}

void Mpris::getPlaybackStatus()
{
    QDBusPendingCall call = getProperty(m_service, mprisPath, mprisPlayerInterface, QStringLiteral("PlaybackStatus"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
    watcher->connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariant> reply = *watcher;

        QString st;
        if (reply.isError()) {
            qDebug("Failed to retrieve mpris metadata.");
        } else {
            st = reply.value().toString();
        }
        updatePlaybackStatus(st);
    });
}

void Mpris::updatePlaybackStatus(const QString &st)
{
    PlaybackStatus old = m_playbackStatus;

    if (st == "Playing") {
        m_playbackStatus = PlaybackStatus::Playing;
    } else if (st == "Paused") {
        m_playbackStatus = PlaybackStatus::Paused;
    } else {
        m_playbackStatus = PlaybackStatus::Stopped;
    }
    if (m_playbackStatus != old) {
        emit playbackStatusChanged();
    }
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
}
