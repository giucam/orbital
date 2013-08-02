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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHELLITEM_H
#define SHELLITEM_H

#include <QQuickWindow>

class ShellItem : public QQuickWindow
{
    Q_OBJECT
    Q_PROPERTY(Type type READ type WRITE setType)
public:
    enum Type {
        None,
        Background,
        Panel,
        Overlay
    };
    Q_ENUMS(Type)
    ShellItem(QWindow *parent = nullptr);

    inline Type type() const { return m_type; }
    inline void setType(Type t) { m_type = t; }

private:
    Type m_type;
};

#endif
