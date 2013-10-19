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

#ifndef SHELLUI_H
#define SHELLUI_H

#include <QObject>
#include <QStringList>

class QXmlStreamReader;
class QXmlStreamWriter;
class QQuickItem;
class QQmlEngine;
class QScreen;

struct wl_output;

class Element;
class Client;
class UiScreen;
class Style;

class ShellUI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme)
    Q_PROPERTY(int numWorkspaces READ numWorkspaces WRITE setNumWorkspaces)
    Q_PROPERTY(bool configMode READ configMode WRITE setConfigMode NOTIFY configModeChanged)
    Q_PROPERTY(QString styleName READ styleName WRITE setStyleName)
public:
    ShellUI(Client *client, QQmlEngine *engine, const QString &configFile);
    ~ShellUI();

    UiScreen *loadScreen(int id, QScreen *screen);
    QQmlEngine *qmlEngine() const { return m_engine; }

    QString iconTheme() const;
    void setIconTheme(const QString &theme);

    int numWorkspaces() const { return m_numWorkspaces; }
    void setNumWorkspaces(int n);

    bool configMode() const { return m_configMode; }
    void setConfigMode(bool mode);

    Style *style() const { return m_style; }

    QString styleName() const { return m_styleName; }
    void setStyleName(const QString &style);

    Q_INVOKABLE void setOverrideCursorShape(Qt::CursorShape shape);
    Q_INVOKABLE void restoreOverrideCursorShape();

    Q_INVOKABLE Element *createElement(const QString &name);
    Q_INVOKABLE void toggleConfigMode();

    UiScreen *findScreen(wl_output *output) const;

public slots:
    void reloadConfig();
    void saveConfig();

signals:
    void configModeChanged();

private:
    void loadScreen(UiScreen *s);

    Client *m_client;
    QString m_configFile;
    QByteArray m_configData;
    bool m_configMode;
    int m_cursorShape;
    QQmlEngine *m_engine;
    QList<UiScreen *> m_screens;
    int m_numWorkspaces;
    QString m_styleName;
    Style *m_style;

    QStringList m_properties;
};

#endif
