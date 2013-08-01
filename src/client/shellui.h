
#ifndef SHELLUI_H
#define SHELLUI_H

#include <QObject>
#include <QQmlListProperty>

#include "shellitem.h"

class QXmlStreamReader;

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

private:
    void loadElement(QQmlEngine *engine, QObject *parent, QXmlStreamReader &xml);

    Client *m_client;
};

#endif
