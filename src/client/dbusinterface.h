/*
 * Copyright 2014-2015 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_DBUSINTERFACE_H
#define ORBITAL_DBUSINTERFACE_H

#include <QDBusAbstractInterface>

/* QDBusInterface uses blocking calls in its constructor.
 * If the remote app is slow it may block us too, so that is not acceptable.
 * QDBusAbstractInterface does not block, but it only has a protected constructor
 * so subclass it.
 * https://bugreports.qt.io/browse/QTBUG-14485
 */
class DBusInterface : public QDBusAbstractInterface
{
public:
    DBusInterface(const QString &service, const QString &path, const QString &interface,
                  const QDBusConnection &connection = QDBusConnection::sessionBus())
        : QDBusAbstractInterface(service, path, qPrintable(interface), connection, nullptr)
    { }
};

#endif
