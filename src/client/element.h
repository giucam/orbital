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

#ifndef ELEMENT_H
#define ELEMENT_H

#include <QQuickItem>
#include <QStringList>

class QQmlEngine;
class QQuickWindow;

struct wl_surface;

class LayoutAttached;
class ElementConfig;
class ShellUI;

class Element : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Type type READ type WRITE setType)
    Q_PROPERTY(QStringList saveProperties READ saveProperties WRITE setSaveProperties)
    Q_PROPERTY(LayoutAttached *layoutItem READ layout WRITE setLayout)
    Q_PROPERTY(QString sortProperty READ sortProperty WRITE setSortProperty)
    Q_PROPERTY(ElementConfig *configureItem READ configureItem)
    Q_PROPERTY(QQuickItem *settingsItem READ settingsItem WRITE setSettingsItem)
    Q_PROPERTY(QQmlComponent *childrenConfig READ childrenConfig WRITE setChildrenConfig)
    Q_PROPERTY(QPointF dragOffset READ dragOffset WRITE setDragOffset)
    Q_PROPERTY(QQuickItem *content READ content WRITE setContent)
    Q_PROPERTY(QQuickItem *childrenParent READ childrenParent WRITE setChildrenParent)
    Q_PROPERTY(QQmlComponent *childrenBackground READ background WRITE setBackground)
    Q_CLASSINFO("DefaultProperty", "resources")
public:
    enum Type {
        Item,
        Background,
        Panel,
        Overlay
    };
    Q_ENUMS(Type)
    explicit Element(Element *parent = nullptr);
    ~Element();

    Q_INVOKABLE void addProperty(const QString &name);
    Q_INVOKABLE void destroyElement();
    Q_INVOKABLE void configure();

    inline Type type() const { return m_type; }
    inline void setType(Type t) { m_type = t; }

    QStringList saveProperties() const { return m_ownProperties; }
    void setSaveProperties(const QStringList &list) { m_ownProperties = list; }

    inline LayoutAttached *layout() const { return m_layout; }
    inline void setLayout(LayoutAttached *l) { m_layout = l; }

    inline QString sortProperty() const { return m_sortProperty; }
    inline void setSortProperty(const QString &p) { m_sortProperty = p; }

    inline ElementConfig *configureItem() const { return m_configureItem; }

    QQuickItem *settingsItem() const { return m_settingsItem; }
    void setSettingsItem(QQuickItem *item) { m_settingsItem = item; }

    QQmlComponent *childrenConfig() const { return m_childrenConfig; }
    void setChildrenConfig(QQmlComponent *component) { m_childrenConfig = component; }

    QPointF dragOffset() const { return m_offset; }
    void setDragOffset(const QPointF &pos) { m_offset = pos; }

    QQuickItem *content() const { return m_content; }
    void setContent(QQuickItem *item);

    QQuickItem *childrenParent() const { return m_childrenParent; }
    void setChildrenParent(QQuickItem *item);

    QQmlComponent *background() const { return m_childrenBackground; }
    void setBackground(QQmlComponent *c) { m_childrenBackground = c; }

    static void loadElementsList();
    static Element *create(ShellUI *shell, QQmlEngine *engine, const QString &name, int id = -1);

    Q_INVOKABLE void publish(const QPointF &offset = QPointF());

signals:
    void elementAdded(Element *element, const QPointF &pos, const QPointF &offset);
    void elementEntered(Element *element, const QPointF &pos, const QPointF &offset);
    void elementMoved(Element *element, const QPointF &pos, const QPointF &offset);
    void elementExited(Element *element, const QPointF &pos, const QPointF &offset);

protected:
    void setId(int id);

private slots:
    void focus(wl_surface *surface, int x, int y);
    void motion(uint32_t time, int x, int y);
    void button(uint32_t time, uint32_t button, uint32_t state);

private:
    void setParentElement(Element *parent);
    void sortChildren();
    void createConfig(Element *child);
    void createBackground(Element *child);
    void settingsVisibleChanged(bool visible);

    static void loadElementInfo(const QString &name, const QString &path);

    QString m_typeName;
    Type m_type;
    int m_id;
    ShellUI *m_shell;
    Element *m_parent;
    QList<Element *> m_children;
    QStringList m_properties;
    QStringList m_ownProperties;
    LayoutAttached *m_layout;
    QString m_sortProperty;
    QQuickItem *m_content;
    QQuickItem *m_childrenParent;
    ElementConfig *m_configureItem;
    QQuickItem *m_settingsItem;
    QQuickWindow *m_settingsWindow;
    QQmlComponent *m_childrenConfig;

    QQmlComponent *m_childrenBackground;
    QQuickItem *m_background;

    Element *m_target;
    QPointF m_pos;
    QPointF m_offset;

    static int s_id;

    struct ElementInfo {
        QString name;
        QString path;
        QString prettyName;
        QString qml;
    };
    static QHash<QString, ElementInfo> s_elements;

    friend class ShellUI;
};

class ElementConfig : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Element *element READ element)
public:
    ElementConfig(QQuickItem *parent = nullptr);

    Element *element() const { return m_element; }

private:
    Element *m_element;

    friend class Element;
};

#endif
