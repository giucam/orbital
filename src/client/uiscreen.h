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

#ifndef UISCREEN_H
#define UISCREEN_H

#include <QObject>
#include <QQmlListProperty>
#include <QStringList>
#include <QRect>
#include <QJsonObject>

class QQmlEngine;
class QQuickItem;
class QScreen;

class Element;
class Client;
class ShellUI;
class Style;

class UiScreen : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QRect availableRect READ availableRect NOTIFY availableRectChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loaded)
public:
    UiScreen(ShellUI *ui, Client *client, QScreen *screen, const QString &name);
    ~UiScreen();

    QScreen *screen() const { return m_screen; }
    QString name() const { return m_name; }

    void loadConfig(QJsonObject &config);
    void saveConfig(QJsonObject &config);

    void addElement(Element *elm);
    void removeElement(Element *elm);

    QRect availableRect() const { return m_rect; }
    void setAvailableRect(const QRect &r);

    bool loading() const;

signals:
    void availableRectChanged();
    void loaded();

private slots:
    void screenLoaded();

private:
    Element *loadElement(Element *parent, QJsonObject &config, QHash<int, Element *> *elements);
    void saveProperties(QObject *obj, const QStringList &properties, QJsonObject &config);
    void saveChildren(const QList<Element *> &children, QJsonObject &config);
    void elementDestroyed(QObject *obj);

    Client *m_client;
    ShellUI *m_ui;
    QString m_name;
    QScreen *m_screen;
    QRect m_rect;
    bool m_loading;

    QHash<int, Element *> m_elements;
    QList<Element *> m_children;
};

#endif
