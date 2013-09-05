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

DateTimeService::DateTimeService()
               : Service()
{

}

DateTimeService::~DateTimeService()
{

}

void DateTimeService::init()
{
    startTimer(250);

    setDT(QDateTime::currentDateTime());
}

QString DateTimeService::time() const
{
    return m_dateTime.time().toString();
}

QString DateTimeService::date() const
{
    return m_dateTime.date().toString(Qt::DefaultLocaleShortDate);
}

void DateTimeService::timerEvent(QTimerEvent *event)
{
    QDateTime dt = QDateTime::currentDateTime();
    if (dt.toTime_t() != m_nsecs) {
        setDT(dt);
    }
}

void DateTimeService::setDT(const QDateTime &dt)
{
    m_dateTime = dt;
    m_nsecs = dt.toTime_t();
    emit updated();
}
