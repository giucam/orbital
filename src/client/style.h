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
#include <QMap>
#include <QColor>

class QQmlComponent;
class QQmlEngine;

#define PROPERTY(type, name) \
    type name() const { return m_##name; } \
    void set_##name(type c) { m_##name = c; emit name##Changed(); } \
    private: \
        type m_##name; \
    public:

class Style : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *panelBackground READ panelBackground WRITE set_panelBackground NOTIFY panelBackgroundChanged)
    Q_PROPERTY(QQmlComponent *panelBorder READ panelBorder WRITE set_panelBorder NOTIFY panelBorderChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE set_textColor NOTIFY textColorChanged)
public:
    Style(QObject *p = nullptr);

    PROPERTY(QQmlComponent *, panelBackground)
    PROPERTY(QQmlComponent *, panelBorder)
    PROPERTY(QColor, textColor)

    static Style *loadStyle(const QString &name, QQmlEngine *engine);

    static void loadStylesList();
    static void cleanupStylesList();

signals:
    void panelBackgroundChanged();
    void panelBorderChanged();
    void textColorChanged();

private:
    static void loadStyleInfo(const QString &name, const QString &path);

    struct StyleInfo {
        QString m_name;
        QString m_path;
        QString m_qml;
    };

    static QMap<QString, StyleInfo *> s_styles;
};

#undef PROPERTY

#endif
