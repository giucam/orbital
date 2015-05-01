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

#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>

#include "clipboard.h"

Clipboard::Clipboard(QObject *p)
         : QObject(p)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, &Clipboard::textChanged);
}

QString Clipboard::text() const
{
    return QGuiApplication::clipboard()->text();
}

void Clipboard::setText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

Clipboard *Clipboard::qmlAttachedProperties(QObject *obj)
{
    return new Clipboard(obj);
}
