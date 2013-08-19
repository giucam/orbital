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

#ifndef STYLE_H
#define STYLE_H

#include <QObject>

class QQmlComponent;

#define PROPERTY(name) \
    QQmlComponent *name() const { return m_##name; } \
    void set_##name(QQmlComponent *c) { m_##name = c; emit name##Changed(); } \
    private: \
        QQmlComponent *m_##name; \
    public:

class Style : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *panelBackground READ panelBackground WRITE set_panelBackground NOTIFY panelBackgroundChanged)
public:
    Style(QObject *p = nullptr) : QObject(p) {}

    PROPERTY(panelBackground)

signals:
    void panelBackgroundChanged();
};

#undef PROPERTY

#endif
