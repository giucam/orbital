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

#ifndef MPRISSERVICE_H
#define MPRISSERVICE_H

#include <QQmlExtensionPlugin>

class MprisPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};

class Mpris : public QObject
{
    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
    Q_PROPERTY(quint64 pid READ pid WRITE setPid NOTIFY targetChanged)
    Q_PROPERTY(PlaybackStatus playbackStatus READ playbackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY trackTitleChanged)
    Q_OBJECT
public:
    enum class PlaybackStatus {
        Stopped,
        Playing,
        Paused
    };
    Q_ENUMS(PlaybackStatus)

    Mpris(QObject *p = nullptr);
    ~Mpris();

    bool isValid() const;
    quint64 pid() const;
    void setPid(quint64 pid);
    inline PlaybackStatus playbackStatus() const { return m_playbackStatus; }
    inline QString trackTitle() const { return m_trackTitle; }

public slots:
    void playPause();
    void stop();
    void previous();
    void next();

signals:
    void validChanged();
    bool targetChanged();
    void playbackStatusChanged();
    void trackTitleChanged();

private slots:
    void propertiesChanged(const QString &service, const QMap<QString, QVariant> &, const QStringList &);

private:
    void checkConnection();
    void checkServices(const QStringList &services);
    void checkService(const QString &service);
    void setValid(bool v);
    void getMetadata();
    void updateMetadata(const QVariantMap &md);
    void getPlaybackStatus();
    void updatePlaybackStatus(const QString &st);

    bool m_valid;
    quint64 m_pid;
    QString m_service;
    PlaybackStatus m_playbackStatus;
    QString m_trackTitle;
};

#endif
