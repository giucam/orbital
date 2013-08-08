
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
{
}

ShellUI::~ShellUI()
{

}

void ShellUI::loadUI(QQmlEngine *engine, const QString &configFile)
{
    m_engine = engine;
    QXmlStreamReader xml;

    QFile file(configFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot find" << configFile;
        xml.addData(defaultConfig);
    } else {
        xml.setDevice(&file);
    }

    m_configFile = configFile;
    engine->rootContext()->setContextProperty("Ui", this);

    while (!xml.atEnd()) {
        if (!xml.readNextStartElement()) {
            continue;
        }
        if (xml.name() == "element") {
            Element *elm = loadElement(engine, nullptr, xml);
            elm->setParent(this);
            m_children << elm;
        } else if (xml.name() == "property") {
            QXmlStreamAttributes attribs = xml.attributes();
            QString name = attribs.value("name").toString();
            QString value = attribs.value("value").toString();

            setProperty(qPrintable(name), value);
            m_properties << name;
        }
    }
    file.close();
}

Element *ShellUI::loadElement(QQmlEngine *engine, Element *parent, QXmlStreamReader &xml)
{
    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));
    QXmlStreamAttributes attribs = xml.attributes();
    if (!attribs.hasAttribute("type")) {
        return nullptr;
    }

    QString type = attribs.value("type").toString();
    int id = attribs.value("id").toInt();

    Element *elm = Element::create(engine, type, parent, id);

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                loadElement(engine, elm, xml);
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
            return elm;
        }
    }
    return elm;
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
    Element *elm = Element::create(m_engine, name, parent);
    return elm;
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

    while (!xml.atEnd()) {
        xml.readNextStartElement();
        if (xml.name() == "element") {
            reloadElement(xml);
        } else if (xml.name() == "property") {
            QXmlStreamAttributes attribs = xml.attributes();
            QString name = attribs.value("name").toString();
            QString value = attribs.value("value").toString();

            setProperty(qPrintable(name), value);
        }
    }
    file.close();
}

void ShellUI::reloadElement(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attribs = xml.attributes();
    if (!attribs.hasAttribute("type")) {
        return;
    }

    int id = attribs.value("id").toInt();
    Element *elm = m_elements[id];

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                reloadElement(xml);
            } else if (xml.name() == "property") {
                QXmlStreamAttributes attribs = xml.attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();

                QQmlProperty::write(elm, name, value);
            }
        }
        if (xml.isEndElement() && xml.name() == "element") {
            xml.readNext();
            return;
        }
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
