
#include "shellui.h"

#include <QIcon>
#include <QtQml>
#include <QDebug>
#include <QQuickItem>

static const int a = qmlRegisterType<ShellUI>("Orbital", 1, 0, "ShellUI");

ShellUI::ShellUI(QObject *p)
       : QObject(p)
{
}

ShellUI::~ShellUI()
{

}

void ShellUI::loadUI(QQmlEngine *engine, const QString &configFile, const QStringList &searchPath)
{
    QFile file;
    bool open = false;
    for (const QString &path: searchPath) {
        file.setFileName(path + "/" + configFile);
        if (file.open(QIODevice::ReadOnly)) {
            open = true;
            break;
        }
    }

    if (!open) {
        qDebug() << "Cannot find" << configFile;
        return;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNextStartElement();
        if (xml.name() == "element") {
            loadElement(engine, this, xml);
        } else if (xml.name() == "property") {
            QXmlStreamAttributes attribs = xml.attributes();
            QString name = attribs.value("name").toString();
            QString value = attribs.value("value").toString();

            setProperty(qPrintable(name), value);
        }
    }
    file.close();
}

void ShellUI::loadElement(QQmlEngine *engine, QObject *parent, QXmlStreamReader &xml)
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

    QVariant v = parent->property("content");
    QQuickItem *content = parent->property("content").value<QQuickItem *>();
    if (!content && qobject_cast<ShellItem *>(parent)) {
        content = static_cast<ShellItem *>(parent)->contentItem();
    }
    if (content) {
        QQuickItem *item = qobject_cast<QQuickItem*>(obj);
        if (item) {
            item->setParentItem(content);
        } else {
            obj->setParent(content);
        }
    } else {
        obj->setParent(parent);
    }

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "element") {
                loadElement(engine, obj, xml);
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

QString ShellUI::iconTheme() const
{
    return QIcon::themeName();
}

void ShellUI::setIconTheme(const QString &theme)
{
    QIcon::setThemeName(theme);
}

static void appendItem(QQmlListProperty<ShellItem> *prop, ShellItem *o)
{
}

QQmlListProperty<ShellItem> ShellUI::items()
{
    return QQmlListProperty<ShellItem>(this, 0, appendItem, 0, 0, 0);
}
