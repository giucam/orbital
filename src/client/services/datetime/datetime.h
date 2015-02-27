/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef DATETIMESERVICE_H
#define DATETIMESERVICE_H

#include <QDateTime>
#include <QQmlExtensionPlugin>

class DateTimePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};

class DateTime : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString time READ time NOTIFY timeChanged)
    Q_PROPERTY(QString date READ date NOTIFY dateChanged)
public:
    DateTime(QObject *p = nullptr);
    ~DateTime();

    QString time() const;
    QString date() const;

signals:
    void timeChanged();
    void dateChanged();

protected:
    virtual void timerEvent(QTimerEvent *event) override;

private:
    void setDT(const QDateTime &dt);

    QDateTime m_dateTime;
    uint m_nsecs;
};

#endif
