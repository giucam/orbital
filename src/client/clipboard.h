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

#ifndef ORBITAL_CLIPBOARD_H
#define ORBITAL_CLIPBOARD_H

#include <QObject>
#include <QtQml>

class Clipboard : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    Clipboard(QObject *p = nullptr);

    QString text() const;
    void setText(const QString &text);

    static Clipboard *qmlAttachedProperties(QObject *object);

signals:
    void textChanged();

};

QML_DECLARE_TYPE(Clipboard)
QML_DECLARE_TYPEINFO(Clipboard, QML_HAS_ATTACHED_PROPERTIES)

#endif
