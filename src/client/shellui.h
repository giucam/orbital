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

#include "shellitem.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class Client;

class ShellUI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme)
    Q_CLASSINFO("DefaultProperty", "items")

public:
    ShellUI(Client *client);
    ~ShellUI();

    void loadUI(QQmlEngine *engine, const QString &configFile, const QStringList &searchPath);

    QString iconTheme() const;
    void setIconTheme(const QString &theme);

public slots:
    void requestFocus(QQuickItem *item);
    void reloadConfig();
    void saveConfig();

private:
    struct Element {
        QObject *obj;
        QString type;
        int id;
        QList<Element> children;
        QStringList properties;
    };

    void loadElement(QQmlEngine *engine, Element *parent, QXmlStreamReader &xml);
    void reloadElement(QXmlStreamReader &xml);
    void saveElement(Element *elm, QXmlStreamWriter &xml);

    Client *m_client;
    QString m_configFile;

    QHash<int, Element *> m_elements;
    Element m_rootElement;
};

#endif
