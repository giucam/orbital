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

#include "element.h"
#include "client.h"
#include "grab.h"
#include "shellui.h"
#include "uiscreen.h"
#include "styleitem.h"
#include "panel.h"

static const int a = qmlRegisterType<Element>("Orbital", 1, 0, "ElementBase");
static const int b = qmlRegisterType<ElementConfig>("Orbital", 1, 0, "ElementConfig");

int Element::s_id = 0;
QMap<QString, ElementInfo *> Element::s_elements;

Element::Element(Element *parent)
       : QQuickItem(parent)
       , m_shell(nullptr)
       , m_parent(nullptr)
       , m_layout(nullptr)
       , m_contentItem(nullptr)
       , m_content(nullptr)
       , m_childrenParent(nullptr)
       , m_configureItem(nullptr)
       , m_settingsItem(nullptr)
       , m_settingsWindow(nullptr)
       , m_childrenConfig(nullptr)
       , m_screen(nullptr)
       , m_inputRegionSet(false)
       , m_location(Location::Floating)
       , m_childrenBackground(nullptr)
       , m_background(nullptr)
{
    setParentElement(parent);
}

Element::~Element()
{
    setParentElement(nullptr);
    foreach (Element *elm, m_children) {
        elm->m_parent = nullptr;
    }
    delete m_settingsWindow;
    delete m_content;
    delete m_contentItem;
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
    if (m_content) {
        m_content->setWidth(newGeometry.width());
        m_content->setHeight(newGeometry.height());
    }

    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void Element::setContent(QQuickItem *c)
{
    delete m_content;
    m_content = c;
    c->setParentItem(this);

    m_content->setWidth(width());
    m_content->setHeight(height());

    emit contentChanged();
}

void Element::setContentItem(QQuickItem *item)
{
    m_contentItem = item;
    item->setParentItem(m_content);

    emit contentItemChanged();
}

void Element::setChildrenParent(QQuickItem *item)
{
    m_childrenParent = item;
    foreach (Element *elm, m_children) {
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
    if (window() && window()->handle()) {
        Client::client()->setInputRegion(window(), rect);
    }
    emit inputRegionChanged();
}

static void traverse(QObject *object, Element::Location l)
{
    for (QObject *o: object->children()) {
        if (StyleItem *s = qobject_cast<StyleItem *>(o)) {
            s->updateLocation(l);
        }
        traverse(o, l);
    }
}
static void traverse(QQuickItem *object, Element::Location l)
{
    foreach (QQuickItem *o, object->childItems()) {
        if (StyleItem *s = qobject_cast<StyleItem *>(o)) {
            s->updateLocation(l);
        }
        traverse(o, l);
    }
    traverse(static_cast<QObject *>(object), l);
}

void Element::setLocation(Location p)
{
    if (m_location != p) {
        m_location = p;
        emit locationChanged();

        foreach (Element *elm, m_children) {
            elm->m_location = p;
            emit elm->locationChanged();
        }

        traverse(this, m_location);
    }
}

QString Element::prettyName() const
{
    return m_info->prettyName();
}

void Element::addProperty(const QString &name)
{
    if (!m_ownProperties.contains(name) && !m_properties.contains(name)) {
        m_properties << name;
    }
}

void Element::destroyElement()
{
    deleteLater();
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
        m_settingsWindow->setTitle(m_typeName + QLatin1String(" Settings"));

        connect(m_settingsWindow, &QWindow::visibleChanged, this, &Element::settingsVisibleChanged);
        connect(m_settingsWindow, &QWindow::widthChanged, [this](int w) { m_settingsItem->setWidth(w); });
        connect(m_settingsWindow, &QWindow::heightChanged, [this](int h) { m_settingsItem->setHeight(h); });
    } else if (m_settingsWindow->isVisible()) {
        m_settingsWindow->hide();
        return;
    }
    m_settingsWindow->show();
}

void Element::closeSettings()
{
    if (!m_settingsWindow) {
        return;
    }

    m_settingsWindow->hide();
}

void Element::publish(const QPointF &offset)
{
    if (type() == ElementInfo::Type::Item) {
        m_target = nullptr;
        m_offset = offset;
        Grab *grab = Client::createGrab();
        connect(grab, &Grab::focus, this, &Element::focus);
        connect(grab, &Grab::motion, this, &Element::motion);
        connect(grab, &Grab::button, this, &Element::button);
        m_properties.clear();
    }
    emit published();
}

void Element::focus(QQuickWindow *window, int x, int y)
{
    if (m_target) {
        emit m_target->elementExited(this, m_pos, m_offset);
        m_target->window()->unsetCursor();
    }

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
    static_cast<Grab *>(sender())->end();
    if (!m_target) {
        return;
    }

    setParentElement(m_target);
    m_target->m_screen->addElement(this);
    m_target->createConfig(this);
    emit m_target->elementAdded(this, m_pos, m_offset);
    m_target->window()->unsetCursor();
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
        setLocation(parent->m_location);
    } else {
        setParentItem(nullptr);
    }
    setParent(parent);
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

Element *Element::fromItem(QQuickItem *item)
{
    while (true) {
        if (!item) {
            return nullptr;
        }

        Element *elm = qobject_cast<Element *>(item);
        if (elm) {
            return elm;
        }
        item = item->parentItem();
    }
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

Element *Element::create(ShellUI *shell, UiScreen *screen, QQmlEngine *engine, const QString &name, int id)
{
    QElapsedTimer timer;
    timer.start();

    if (!s_elements.contains(name)) {
        qWarning() << QStringLiteral("Could not find the element \'%1\'. Check your configuration or your setup.").arg(name);
        return nullptr;
    }

    ElementInfo *info = s_elements.value(name);
    QQmlComponent c(engine);
    c.loadUrl(QUrl(info->m_qml));
    if (!c.isReady()) {
        qWarning() << "Could not load the element" << name;
        qWarning() << qPrintable(c.errorString());
        return nullptr;
    }

    QObject *obj = c.beginCreate(engine->rootContext());
    Element *elm = qobject_cast<Element *>(obj);
    if (!elm) {
        qWarning() << QStringLiteral("\'%1\' is not an element type.").arg(name);
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
    elm->m_screen = nullptr;
    screen->addElement(elm);

    c.completeCreate();

    qDebug() <<"Creating" << name << "in" << timer.elapsed() << "ms.";

    return elm;
}

void Element::loadElementsList()
{
    auto checkPath = [](const QString &path) {
        QDir dir(QUrl(path).toString(QUrl::NormalizePathSegments));
        const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir: subdirs) {
            dir.cd(subdir);
            if (dir.exists(QStringLiteral("element"))) {
                if (!s_elements.contains(subdir)) {
                    loadElementInfo(subdir, dir.absolutePath());
                }
            }
            dir.cdUp();
        }
    };

    checkPath(QLatin1String(DATA_PATH "/elements"));

    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    foreach (const QString &path, dirs) {
        checkPath(QStringLiteral("%1/../orbital/elements").arg(path));
    }
}

void Element::cleanupElementsList()
{
    foreach (ElementInfo *info, s_elements) {
        delete info;
    }
}

void Element::loadElementInfo(const QString &name, const QString &path)
{
    QString filePath(path + QLatin1String("/element"));
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << QStringLiteral("Failed to load the element '%1'. Could not open %1 for reading.").arg(filePath);
        return;
    }

    ElementInfo *info = new ElementInfo;
    info->m_name = name;
    info->m_path = path;
    info->m_prettyName = name;
    info->m_type = ElementInfo::Type::Item;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning("Error parsing %s at offset %d: %s", qPrintable(filePath), error.offset, qPrintable(error.errorString()));
        delete info;
        return;
    }
    QJsonObject json = doc.object();

    info->m_prettyName = json.value(QStringLiteral("prettyName")).toString();
    if (json.contains(QStringLiteral("qmlFile"))) {
        info->m_qml = path + QLatin1Char('/') + json.value(QStringLiteral("qmlFile")).toString();
    }
    if (json.contains(QStringLiteral("type"))) {
        QString value = json.value(QStringLiteral("type")).toString();
        if (value == QStringLiteral("background")) {
            info->m_type = ElementInfo::Type::Background;
        } else if (value == QStringLiteral("panel")) {
            info->m_type = ElementInfo::Type::Panel;
        } else if (value == QStringLiteral("overlay")) {
            info->m_type = ElementInfo::Type::Overlay;
        } else if (value == QStringLiteral("lockscreen")) {
            info->m_type = ElementInfo::Type::LockScreen;
        }
    }

    if (info->m_qml.isEmpty()) {
        qWarning() << QStringLiteral("Failed to load the element '%1'. Missing 'qmlFile' field.").arg(path);
        delete info;
        return;
    }

    QDir dir(path);
    QTranslator *tr = new QTranslator;
    if (tr->load(Client::locale(), QString(), QString(), path, QStringLiteral(".qm"))) {
        QCoreApplication::installTranslator(tr);
    } else {
        delete tr;
    }

    s_elements.insert(name, info);
}

ElementConfig::ElementConfig(QQuickItem *parent)
             : QQuickItem(parent)
             , m_element(nullptr)
{
}
