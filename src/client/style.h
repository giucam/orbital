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

class Style : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *panelBackground MEMBER m_panelBackground NOTIFY panelBackgroundChanged)
    Q_PROPERTY(QQmlComponent *panelBorder MEMBER m_panelBorder NOTIFY panelBorderChanged)
    Q_PROPERTY(QQmlComponent *taskBarBackground MEMBER m_taskBarBackground NOTIFY taskBarBackgroundChanged)
    Q_PROPERTY(QQmlComponent *taskBarItem MEMBER m_taskBarItem NOTIFY taskBarItemChanged)

    Q_PROPERTY(QQmlComponent *pagerBackground MEMBER m_pagerBackground NOTIFY pagerBackgroundChanged)
    Q_PROPERTY(QQmlComponent *pagerWorkspace MEMBER m_pagerWorkspace NOTIFY pagerWorkspaceChanged)

    Q_PROPERTY(QQmlComponent *toolTipBackground MEMBER m_toolTipBackground NOTIFY toolTipBackgroundChanged)

    Q_PROPERTY(QQmlComponent *button MEMBER m_button NOTIFY buttonChanged)

    Q_PROPERTY(QQmlComponent *popup MEMBER m_popup NOTIFY popupChanged)
    Q_PROPERTY(QQmlComponent *popupLauncher MEMBER m_popupLauncher NOTIFY popupLauncherChanged)

    Q_PROPERTY(QQmlComponent *notificationBackground MEMBER m_notificationBackground NOTIFY notificationBackgroundChanged)

    Q_PROPERTY(QColor textColor MEMBER m_textColor NOTIFY textColorChanged)
    Q_PROPERTY(QColor backgroundColor MEMBER m_backgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor highlightColor MEMBER m_highlightColor NOTIFY highlightColorChanged)
public:
    Style(QObject *p = nullptr);

private:
    QQmlComponent *m_panelBackground;
    QQmlComponent *m_panelBorder;
    QQmlComponent *m_taskBarBackground;
    QQmlComponent *m_taskBarItem;
    QQmlComponent *m_pagerBackground;
    QQmlComponent *m_pagerWorkspace;
    QQmlComponent *m_toolTipBackground;
    QQmlComponent *m_button;
    QQmlComponent *m_popup;
    QQmlComponent *m_popupLauncher;
    QQmlComponent *m_notificationBackground;
    QColor m_textColor;
    QColor m_backgroundColor;
    QColor m_highlightColor;
public:

    static Style *loadStyle(const QString &name, QQmlEngine *engine);

    static void loadStylesList();
    static void cleanupStylesList();
    static QMap<QString, StyleInfo *> stylesInfo() { return s_styles; }

signals:
    void panelBackgroundChanged();
    void panelBorderChanged();
    void taskBarBackgroundChanged();
    void taskBarItemChanged();
    void pagerBackgroundChanged();
    void pagerWorkspaceChanged();
    void toolTipBackgroundChanged();
    void buttonChanged();
    void popupChanged();
    void popupLauncherChanged();
    void notificationBackgroundChanged();
    void textColorChanged();
    void backgroundColorChanged();
    void highlightColorChanged();

private:
    static void loadStyleInfo(const QString &name, const QString &path);

    static QMap<QString, StyleInfo *> s_styles;
};

#endif
