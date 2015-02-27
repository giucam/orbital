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

#include "datetime.h"

#include <QDebug>
#include <QtQml>

void DateTimePlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<DateTime>(uri, 1, 0, "DateTime", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(new DateTime);
    });
}


DateTime::DateTime(QObject *p)
        : QObject(p)
{
    startTimer(250);

    setDT(QDateTime::currentDateTime());
}

DateTime::~DateTime()
{

}

QString DateTime::time() const
{
    return m_dateTime.time().toString();
}

QString DateTime::date() const
{
    return m_dateTime.date().toString(Qt::DefaultLocaleShortDate);
}

void DateTime::timerEvent(QTimerEvent *event)
{
    QDateTime dt = QDateTime::currentDateTime();
    if (dt.toTime_t() != m_nsecs) {
        setDT(dt);
    }
}

void DateTime::setDT(const QDateTime &dt)
{
    QString d = date();
    m_dateTime = dt;
    m_nsecs = dt.toTime_t();

    emit timeChanged();
    if (d != date()) {
        emit dateChanged();
    }
}
