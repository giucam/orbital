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

#include <QtQml>
#include <QGuiApplication>
#include <QQuickWindow>
#include <qpa/qplatformnativeinterface.h>

#include "element.h"
#include "client.h"
#include "grab.h"
#include "shellui.h"
#include "uiscreen.h"

static const int a = qmlRegisterType<Element>("Orbital", 1, 0, "Element");
static const int b = qmlRegisterType<ElementConfig>("Orbital", 1, 0, "ElementConfig");

int Element::s_id = 0;
QMap<QString, ElementInfo *> Element::s_elements;

Element::Element(Element *parent)
       : QQuickItem(parent)
       , m_shell(nullptr)
       , m_parent(nullptr)
       , m_layout(nullptr)
       , m_contentItem(nullptr)
       , m_content(new QQuickItem(this))
       , m_childrenParent(nullptr)
       , m_configureItem(nullptr)
       , m_settingsItem(nullptr)
       , m_settingsWindow(nullptr)
       , m_childrenConfig(nullptr)
       , m_screen(nullptr)
       , m_style(nullptr)
       , m_inputRegionSet(false)
       , m_childrenBackground(nullptr)
       , m_background(nullptr)
{
    setParentElement(parent);
}

Element::~Element()
{
    if (m_parent) {
        m_parent->m_children.removeOne(this);
    }
    for (Element *elm: m_children) {
        elm->m_parent = nullptr;
    }
    delete m_settingsWindow;
}

void Element::setId(int id)
{
    m_id = id;
    if (id >= s_id) {
        s_id = id + 1;
    }
}

void Element::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_content->setWidth(newGeometry.width());
    m_content->setHeight(newGeometry.height());

    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void Element::setContentItem(QQuickItem *item)
{
    m_contentItem = item;
    item->setParentItem(m_content);
}

void Element::setChildrenParent(QQuickItem *item)
{
    m_childrenParent = item;
    for (Element *elm: m_children) {
        elm->setParentItem(m_childrenParent);
    }
}

QRectF Element::inputRegion() const
{
    if (m_inputRegionSet) {
        return m_inputRegion;
    }

    return QRectF(0, 0, width(), height());
}

void Element::setInputRegion(const QRectF &rect)
{
    m_inputRegion = rect;
    m_inputRegionSet = true;
}

void Element::addProperty(const QString &name)
{
    if (!m_ownProperties.contains(name) && !m_properties.contains(name)) {
        m_properties << name;
    }
}

void Element::destroyElement()
{
    delete this;
}

void Element::configure()
{
    if (!m_settingsItem) {
        return;
    }

    if (!m_settingsWindow) {
        m_settingsWindow = Client::createUiWindow();
        m_settingsItem->setParentItem(m_settingsWindow->contentItem());

        m_settingsWindow->setWidth(m_settingsItem->width());
        m_settingsWindow->setHeight(m_settingsItem->height());
        m_settingsWindow->setTitle(m_typeName + " Settings");

        connect(m_settingsWindow, &QWindow::visibleChanged, this, &Element::settingsVisibleChanged);
    } else if (m_settingsWindow->isVisible()) {
        m_settingsWindow->hide();
        return;
    }
    m_settingsWindow->show();
}

void Element::publish(const QPointF &offset)
{
    m_target = nullptr;
    m_offset = offset;
    Grab *grab = Client::createGrab();
    connect(grab, SIGNAL(focus(wl_surface *, int, int)), this, SLOT(focus(wl_surface *, int, int)));
    connect(grab, &Grab::motion, this, &Element::motion);
    connect(grab, &Grab::button, this, &Element::button);
    m_properties.clear();
}

void Element::focus(wl_surface *surface, int x, int y)
{
    if (m_target) {
        emit m_target->elementExited(this, m_pos, m_offset);
        m_target->window()->unsetCursor();
    }

    QQuickWindow *window = Client::client()->findWindow(surface);
    QQuickItem *item = window->contentItem()->childAt(x, y);
    m_target = nullptr;
    while (!m_target && item) {
        m_target = qobject_cast<Element *>(item);
        item = item->parentItem();
    }
    window->setCursor(QCursor(Qt::DragMoveCursor));

    m_pos = QPointF(x, y);
    if (m_target) {
        m_target->createBackground(this);
        emit m_target->elementEntered(this, m_pos, m_offset);
        emit m_target->elementMoved(this, m_pos, m_offset);
    }
}

void Element::motion(uint32_t time, int x, int y)
{
    m_pos = QPointF(x, y);

    if (m_target) {
        emit m_target->elementMoved(this, m_pos, m_offset);
    }
}

void Element::button(uint32_t time, uint32_t button, uint32_t state)
{
    if (m_target) {
        setParentElement(m_target);
        m_target->m_screen->addElement(this);
        m_target->createConfig(this);
        emit m_target->elementAdded(this, m_pos, m_offset);
    } else {
        delete this;
    }
    m_target->window()->unsetCursor();

    static_cast<Grab *>(sender())->end();
}

void Element::setStyle(Style *s)
{
    m_style = s;
    emit styleChanged();
}

void Element::setParentElement(Element *parent)
{
    if (parent == m_parent) {
        return;
    }

    if (m_parent) {
        m_parent->m_children.removeOne(this);
    }

    if (parent) {
        QQuickItem *parentItem = parent->m_childrenParent;
        if (!parentItem) {
            parentItem = parent;
        }
        setParentItem(parentItem);
        parent->m_children << this;
    } else {
        setParentItem(nullptr);
    }
    m_parent = parent;
}

void Element::sortChildren()
{
    if (m_sortProperty.isNull()) {
        return;
    }

    struct Sorter {
         QString property;

        bool operator()(Element *a, Element *b) {
            return QQmlProperty::read(a, property).toInt() < QQmlProperty::read(b, property).toInt();
        }
    };

    Sorter sorter = { m_sortProperty };
    qSort(m_children.begin(), m_children.end(), sorter);
}

void Element::createConfig(Element *child)
{
    delete child->m_configureItem;
    child->m_configureItem = nullptr;
    if (m_childrenConfig) {
        QObject *obj = m_childrenConfig->beginCreate(m_shell->qmlEngine()->rootContext());
        if (ElementConfig *e = qobject_cast<ElementConfig *>(obj)) {
            e->setParentItem(child);
            e->m_element = child;
            child->m_configureItem = e;
            m_childrenConfig->completeCreate();
        } else {
            qWarning("childrenConfig must be a ElementConfig!");
            delete obj;
        }
    }
}

void Element::createBackground(Element *child)
{
    delete child->m_background;
    child->m_background = nullptr;
    if (m_childrenBackground) {
        QObject *obj = m_childrenBackground->beginCreate(m_shell->qmlEngine()->rootContext());
        if (QQuickItem *e = qobject_cast<QQuickItem *>(obj)) {
            e->setParentItem(child);
            e->setZ(-1);
            child->m_background = e;
            m_childrenBackground->completeCreate();
        } else {
            qWarning("childrenBackground must be a QQuickItem!");
            delete obj;
        }
    }
}

void Element::settingsVisibleChanged(bool visible)
{
    if (!visible) {
        m_shell->saveConfig();
    }
}

Element *Element::create(ShellUI *shell, QQmlEngine *engine, const QString &name, Style *style, int id)
{
    QElapsedTimer timer;
    timer.start();

    if (!s_elements.contains(name)) {
        qWarning() << QString("Could not find the element \'%1\'. Check your configuration or your setup.").arg(name);
        return nullptr;
    }

    ElementInfo *info = s_elements.value(name);
    QQmlComponent c(engine);
    c.loadUrl(info->m_qml);
    if (!c.isReady()) {
        qWarning() << "Could not load the element" << name;
        qWarning() << qPrintable(c.errorString());
        return nullptr;
    }

    QObject *obj = c.beginCreate(engine->rootContext());
    Element *elm = qobject_cast<Element *>(obj);
    if (!elm) {
        qWarning() << QString("\'%1\' is not an element type.").arg(name);
        delete obj;
        return nullptr;
    }

    if (id < 0) {
        elm->m_id = s_id++;
    } else {
        elm->setId(id);
    }
    elm->m_typeName = name;
    elm->m_shell = shell;
    elm->m_info = info;
    elm->m_style = style;

    c.completeCreate();

    qDebug() <<"Creating" << name << "in" << timer.elapsed() << "ms.";

    return elm;
}

void Element::loadElementsList()
{
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::DataLocation, "elements", QStandardPaths::LocateDirectory);

    for (const QString &path: dirs) {
        QDir dir(path);
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir: subdirs) {
            dir.cd(subdir);
            if (dir.exists("element")) {
                if (!s_elements.contains(subdir)) {
                    loadElementInfo(subdir, dir.absolutePath());
                }
            }
            dir.cdUp();
        }
    }
}

void Element::cleanupElementsList()
{
    for (ElementInfo *info: s_elements) {
        delete info;
    }
}

void Element::loadElementInfo(const QString &name, const QString &path)
{
    QString filePath(path + "/element");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << QString("Failed to load the element '%1'. Could not open %1 for reading.").arg(filePath);
        return;
    }

    ElementInfo *info = new ElementInfo;
    info->m_name = name;
    info->m_path = path;
    info->m_prettyName = name;
    info->m_type = ElementInfo::Type::Item;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();

        QStringList parts = line.split('=');
        if (parts.size() < 2) {
            continue;
        }
        const QString &key = parts.at(0);
        const QString &value = parts.at(1);
        if (key == "prettyName") {
            info->m_prettyName = value;
        } else if (key == "qmlFile") {
            info->m_qml = path + "/" + value;
        } else if (key == "type") {
            if (value == "background") {
                info->m_type = ElementInfo::Type::Background;
            } else if (value == "panel") {
                info->m_type = ElementInfo::Type::Panel;
            } else if (value == "overlay") {
                info->m_type = ElementInfo::Type::Overlay;
            }
        }
    };
    file.close();

    if (info->m_qml.isEmpty()) {
        qWarning() << QString("Failed to load the element '%1'. Missing 'qmlFile' field.").arg(path);
        delete info;
        return;
    }

    s_elements.insert(name, info);
}

ElementConfig::ElementConfig(QQuickItem *parent)
             : QQuickItem(parent)
             , m_element(nullptr)
{
}
