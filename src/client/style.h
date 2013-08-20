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

class StyleInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString prettyName READ prettyName CONSTANT)
public:
    StyleInfo() {}

    QString prettyName() const { return m_prettyName; }
    QString name() const { return m_name; }

private:
    QString m_prettyName;
    QString m_name;
    QString m_path;
    QString m_qml;

    friend class Style;
};

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
    Q_PROPERTY(QQmlComponent *taskBarBackground READ taskBarBackground WRITE set_taskBarBackground NOTIFY taskBarBackgroundChanged)
    Q_PROPERTY(QQmlComponent *taskBarItem READ taskBarItem WRITE set_taskBarItem NOTIFY taskBarItemChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE set_textColor NOTIFY textColorChanged)
public:
    Style(QObject *p = nullptr);

    PROPERTY(QQmlComponent *, panelBackground)
    PROPERTY(QQmlComponent *, panelBorder)
    PROPERTY(QQmlComponent *, taskBarBackground)
    PROPERTY(QQmlComponent *, taskBarItem)
    PROPERTY(QColor, textColor)

    static Style *loadStyle(const QString &name, QQmlEngine *engine);

    static void loadStylesList();
    static void cleanupStylesList();
    static QMap<QString, StyleInfo *> stylesInfo() { return s_styles; }

signals:
    void panelBackgroundChanged();
    void panelBorderChanged();
    void taskBarBackgroundChanged();
    void taskBarItemChanged();
    void textColorChanged();

private:
    static void loadStyleInfo(const QString &name, const QString &path);

    static QMap<QString, StyleInfo *> s_styles;
};

#undef PROPERTY

#endif
