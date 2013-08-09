
#include "shellui.h"

#include <QIcon>
#include <QDebug>
#include <QQuickItem>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQmlProperty>
#include <QCoreApplication>
#include <QQuickWindow>

#include "client.h"
#include "element.h"

static const char *defaultConfig =
"<Ui>\n"
"    <property name=\"iconTheme\" value=\"oxygen\"/>\n"
"    <element type=\"Background\" id=\"1\">\n"
"        <property name=\"color\" value=\"black\"/>\n"
"        <property name=\"imageSource\" value=\"/usr/share/weston/pattern.png\"/>\n"
"        <property name=\"imageFillMode\" value=\"4\"/>\n"
"    </element>\n"
"   <element type=\"Panel\" id=\"2\">\n"
"        <element type=\"Launcher\" id=\"3\">\n"
"            <property name=\"icon\" value=\"image://icon/utilities-terminal\"/>\n"
"            <property name=\"process\" value=\"/usr/bin/weston-terminal\"/>\n"
"        </element>\n"
"        <element type=\"TaskBar\" id=\"4\"/>\n"
"        <element type=\"Logout\" id=\"5\"/>\n"
"        <element type=\"Clock\" id=\"6\"/>\n"
"    </element>\n"
"    <element type=\"Overlay\" id=\"7\"/>\n"
"</Ui>\n";

ShellUI::ShellUI(Client *client)
       : QObject(client)
       , m_client(client)
       , m_configMode(false)
{
}

ShellUI::~ShellUI()
{

}

void ShellUI::loadUI(QQmlEngine *engine, const QString &configFile)
{
    m_engine = engine;
    m_configFile = configFile;
    engine->rootContext()->setContextProperty("Ui", this);

    reloadConfig();
}

Element *ShellUI::loadElement(Element *parent, QXmlStreamReader &xml, QHash<int, Element *> *elements)
{
    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));
    QXmlStreamAttributes attribs = xml.attributes();
    if (!attribs.hasAttribute("type")) {
        return nullptr;
    }

    bool created = false;
    int id = attribs.value("id").toInt();
    Element *elm = (elements ? elements->take(id) : nullptr);
    if (!elm) {
        QString type = attribs.value("type").toString();
        elm = Element::create(m_engine, type, id);
        connect(elm, &QObject::destroyed, this, &ShellUI::elementDestroyed);
        created = true;
    }
    if (parent) {
        elm->setParentElement(parent);
    }
    elm->m_properties.clear();

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                loadElement(elm, xml, elements);
            } else if (xml.name() == "property") {
                QXmlStreamAttributes attribs = xml.attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();

                QQmlProperty::write(elm, name, value);
                elm->addProperty(name);
            }
        }
        if (xml.isEndElement() && xml.name() == "element") {
            xml.readNext();
            m_elements.insert(id, elm);
            if (created && parent) {
                parent->createConfig(elm);
            }
            return (created ? elm : nullptr);
        }
    }
    return (created ? elm : nullptr);
}

QString ShellUI::iconTheme() const
{
    return QIcon::themeName();
}

void ShellUI::setIconTheme(const QString &theme)
{
    QIcon::setThemeName(theme);
}

Element *ShellUI::createElement(const QString &name, Element *parent)
{
    Element *elm = Element::create(m_engine, name);
    elm->setParentElement(parent);
    connect(elm, &QObject::destroyed, this, &ShellUI::elementDestroyed);
    return elm;
}

void ShellUI::setConfigMode(bool mode)
{
    m_configMode = mode;
    emit configModeChanged();
    if (!m_configMode) {
        saveConfig();
    }
}

void ShellUI::toggleConfigMode()
{
    setConfigMode(!m_configMode);
}

void ShellUI::requestFocus(QQuickItem *item)
{
    m_client->requestFocus(item->window());
}

void ShellUI::reloadConfig()
{
    QXmlStreamReader xml;

    QFile file(m_configFile);
    if (!file.open(QIODevice::ReadOnly)) {
        xml.addData(defaultConfig);
    } else {
        xml.setDevice(&file);
    }

    for (Element *elm: m_elements) {
        if (elm->m_parent) {
            elm->setParentElement(nullptr);
        }
    }

    QHash<int, Element *> oldElements = m_elements;
    m_elements.clear();
    m_properties.clear();

    while (!xml.atEnd()) {
        if (!xml.readNextStartElement()) {
            continue;
        }
        if (xml.name() == "element") {
            Element *elm = loadElement(nullptr, xml, &oldElements);
            if (elm) {
                elm->setParent(this);
                m_children << elm;
            }
        } else if (xml.name() == "property") {
            QXmlStreamAttributes attribs = xml.attributes();
            QString name = attribs.value("name").toString();
            QString value = attribs.value("value").toString();

            setProperty(qPrintable(name), value);
            m_properties << name;
        }
    }
    file.close();

    for (Element *e: oldElements) {
        delete e;
        m_children.removeOne(e);
    }
}

void ShellUI::saveConfig()
{
    QFile file(m_configFile);
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xml(&file);

    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("Ui");

    saveProperties(this, m_properties, xml);
    saveChildren(m_children, xml);

    xml.writeEndElement();
    file.close();
}

void ShellUI::saveElement(Element *elm, QXmlStreamWriter &xml)
{
    saveProperties(elm, elm->m_properties, xml);
    elm->sortChildren();
    saveChildren(elm->m_children, xml);
}

void ShellUI::saveProperties(QObject *obj, const QStringList &properties, QXmlStreamWriter &xml)
{
    for (const QString &prop: properties) {
        xml.writeStartElement("property");
        xml.writeAttribute("name", prop);
        xml.writeAttribute("value", obj->property(qPrintable(prop)).toString());
        xml.writeEndElement();
    }
}

void ShellUI::saveChildren(const QList<Element *> &children, QXmlStreamWriter &xml)
{
    for (Element *child: children) {
        xml.writeStartElement("element");
        xml.writeAttribute("type", child->m_typeName);
        xml.writeAttribute("id", QString::number(child->m_id));

        saveElement(child, xml);

        xml.writeEndElement();
    }

}

void ShellUI::elementDestroyed(QObject *obj)
{
    Element *elm = static_cast<Element *>(obj);
    m_children.removeOne(elm);
    m_elements.remove(elm->m_id);
}
