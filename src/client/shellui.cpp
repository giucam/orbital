
#include "shellui.h"

#include <QIcon>
#include <QDebug>
#include <QQuickItem>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QQmlEngine>
#include <QQmlContext>
#include <QCoreApplication>

#include "client.h"

static const char *defaultConfig =
"<Ui>\n"
"    <property name=\"iconTheme\" value=\"oxygen\"/>\n"
"    <element type=\"Background\" id=\"1\">\n"
"        <property name=\"color\" value=\"black\"/>\n"
"        <property name=\"imageSource\" value=\"/usr/share/weston/pattern.png\"/>\n"
"        <property name=\"imageFillMode\" value=\"3\"/>\n"
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

    m_rootElement.obj = this;
    m_rootElement.id = 0;

    while (!xml.atEnd()) {
        if (!xml.readNextStartElement()) {
            continue;
        }
        if (xml.name() == "element") {
            loadElement(engine, &m_rootElement, xml);
        } else if (xml.name() == "property") {
            QXmlStreamAttributes attribs = xml.attributes();
            QString name = attribs.value("name").toString();
            QString value = attribs.value("value").toString();

            setProperty(qPrintable(name), value);
            m_rootElement.properties << name;
        }
    }
    file.close();
}

void ShellUI::loadElement(QQmlEngine *engine, Element *parent, QXmlStreamReader &xml)
{
    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));
    QXmlStreamAttributes attribs = xml.attributes();
    if (!attribs.hasAttribute("type")) {
        return;
    }

    QStringRef type = attribs.value("type");

    QQmlComponent *c = new QQmlComponent(engine, this);
    c->loadUrl(path + type.toString() + ".qml");
    if (!c->isReady())
        qFatal(qPrintable(c->errorString()));

    QObject *obj = c->create();
    int id = attribs.value("id").toInt();
    obj->setObjectName(QString("element_%1").arg(attribs.value("id").toString()));

    QVariant v = parent->obj->property("content");
    QQuickItem *content = parent->obj->property("content").value<QQuickItem *>();
    if (!content && qobject_cast<ShellItem *>(parent->obj)) {
        content = static_cast<ShellItem *>(parent->obj)->contentItem();
    }
    if (content) {
        QQuickItem *item = qobject_cast<QQuickItem*>(obj);
        if (item) {
            item->setParentItem(content);
        } else {
            obj->setParent(content);
        }
    } else {
        obj->setParent(parent->obj);
    }

    Element elm;
    elm.obj = obj;
    elm.type = type.toString();
    elm.id = id;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                loadElement(engine, &elm, xml);
            } else if (xml.name() == "property") {
                QXmlStreamAttributes attribs = xml.attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();

                obj->setProperty(qPrintable(name), value);
                elm.properties << name;
            }
        }
        if (xml.isEndElement() && xml.name() == "element") {
            xml.readNext();
            parent->children << elm;
            m_elements.insert(id, &parent->children.last());
            return;
        }
    }
}

QString ShellUI::iconTheme() const
{
    return QIcon::themeName();
}

void ShellUI::setIconTheme(const QString &theme)
{
    QIcon::setThemeName(theme);
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
    QObject *obj = m_elements[id]->obj;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                reloadElement(xml);
            } else if (xml.name() == "property") {
                QXmlStreamAttributes attribs = xml.attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();

                obj->setProperty(qPrintable(name), value);
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

    saveElement(&m_rootElement, xml);

    xml.writeEndElement();
    file.close();
}

void ShellUI::saveElement(Element *elm, QXmlStreamWriter &xml)
{
    QObject *obj = elm->obj;
    for (const QString &prop: elm->properties) {
        xml.writeStartElement("property");
        xml.writeAttribute("name", prop);
        xml.writeAttribute("value", obj->property(qPrintable(prop)).toString());
        xml.writeEndElement();
    }

    for (Element &child: elm->children) {
        xml.writeStartElement("element");
        xml.writeAttribute("type", child.type);
        xml.writeAttribute("id", QString::number(child.id));

        saveElement(&child, xml);

        xml.writeEndElement();
    }
}
