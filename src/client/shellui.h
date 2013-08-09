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

#ifndef SHELLUI_H
#define SHELLUI_H

#include <QObject>
#include <QQmlListProperty>
#include <QStringList>

class QXmlStreamReader;
class QXmlStreamWriter;
class QQmlEngine;
class QQuickItem;

class Element;
class Client;

class ShellUI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme)
    Q_PROPERTY(bool configMode READ configMode WRITE setConfigMode NOTIFY configModeChanged)
public:
    ShellUI(Client *client);
    ~ShellUI();

    void loadUI(QQmlEngine *engine, const QString &configFile);

    QString iconTheme() const;
    void setIconTheme(const QString &theme);

    bool configMode() const { return m_configMode; }
    void setConfigMode(bool mode);

    Q_INVOKABLE Element *createElement(const QString &name, Element *parent);
    Q_INVOKABLE void toggleConfigMode();

public slots:
    void requestFocus(QQuickItem *item);
    void reloadConfig();
    void saveConfig();

signals:
    void configModeChanged();

private:
    Element *loadElement(Element *parent, QXmlStreamReader &xml, QHash<int, Element *> *elements);
    void saveElement(Element *elm, QXmlStreamWriter &xml);
    void saveProperties(QObject *obj, const QStringList &properties, QXmlStreamWriter &xml);
    void saveChildren(const QList<Element *> &children, QXmlStreamWriter &xml);
    void elementDestroyed(QObject *obj);

    Client *m_client;
    QQmlEngine *m_engine;
    QString m_configFile;
    bool m_configMode;

    QHash<int, Element *> m_elements;
    QList<Element *> m_children;
    QStringList m_properties;
};

#endif
