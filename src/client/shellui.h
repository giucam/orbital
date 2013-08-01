
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
