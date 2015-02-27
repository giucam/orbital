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

#ifndef NOTIFICATIONSSERVICE_H
#define NOTIFICATIONSSERVICE_H

#include <QDBusAbstractAdaptor>
#include <QPixmap>
#include <QQmlExtensionPlugin>

class NotificationsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};

class Notification : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QString summary READ summary CONSTANT)
    Q_PROPERTY(QString body READ body CONSTANT)

public:
    Notification();

    int id() const { return m_id; }
    QString summary() const { return m_summary; }
    QString body() const { return m_body; }
    QString iconName() const { return m_iconName; }
    QPixmap iconImage() const { return m_iconImage; }

    void setId(int id);
    void setSummary(const QString &s);
    void setBody(const QString &body);
    void setIconName(const QString &icon);
    void setIconImage(const QPixmap &img);

protected:
    void timerEvent(QTimerEvent *e);

signals:
    void expired();

private:
    int m_id;
    QString m_summary;
    QString m_body;
    QString m_iconName;
    QPixmap m_iconImage;
};

class NotificationsManager : public QObject
{
    Q_OBJECT
public:
    NotificationsManager(QObject *p = nullptr);
    ~NotificationsManager();

    void newNotification(Notification *notification);

    Notification *notification(int id) const;

signals:
    void notify(Notification *notification);

private:
    QHash<int, Notification *> m_notifications;
};

#endif
