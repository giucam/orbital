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

#ifndef STATUSNOTIFIERITEM_H
#define STATUSNOTIFIERITEM_H

#include <QObject>
#include <QVector>
#include <QDBusInterface>

//Custom message type for DBus
struct DBusImageStruct {
    int width;
    int height;
    QByteArray data;
};

typedef QVector<DBusImageStruct> DBusImageVector;

struct DBusToolTipStruct {
    QString icon;
    DBusImageVector image;
    QString title;
    QString subTitle;
};

class StatusNotifierItem: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(QString attentionIconName READ attentionIconName NOTIFY attentionIconNameChanged)
    Q_PROPERTY(QString tooltipTitle READ tooltipTitle NOTIFY tooltipChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
public:
    enum class Status {
        Passive,
        Active,
        NeedsAttention
    };
    Q_ENUMS(Status)

    explicit StatusNotifierItem(const QString &service, QObject *parent = nullptr);
    ~StatusNotifierItem();

    QString name() const;
    QString title() const;
    QString iconName() const;
    QString attentionIconName() const;
    QString tooltipTitle() const;
    Status status() const;

    Q_INVOKABLE void activate();
    Q_INVOKABLE void secondaryActivate();
    Q_INVOKABLE void scroll(const QPoint &delta);

signals:
    void removed();
    void nameChanged();
    void titleChanged();
    void iconNameChanged();
    void attentionIconNameChanged();
    void tooltipChanged();
    void statusChanged();

private slots:
    void getTitle();
    void getIcon();
    void getAttentionIcon();
    void getTooltip();
    void getStatus();
private:
    QString m_service;
    QString m_name;
    QString m_title;
    QString m_iconName;
    QString m_attentionIconName;
    DBusToolTipStruct m_tooltip;
    Status m_status;
    QDBusInterface m_interface;
};

#endif
