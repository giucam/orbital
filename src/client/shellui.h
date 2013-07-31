
#ifndef SHELLUI_H
#define SHELLUI_H

#include <QObject>
#include <QQmlListProperty>

#include "shellitem.h"

class QXmlStreamReader;

class ShellUI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme)
    Q_PROPERTY(QQmlListProperty<ShellItem> items READ items)
    Q_CLASSINFO("DefaultProperty", "items")

public:
    ShellUI(QObject *p = nullptr);
    ~ShellUI();

    void loadUI(QQmlEngine *engine, const QString &configFile, const QStringList &searchPath);

    QString iconTheme() const;
    void setIconTheme(const QString &theme);

    QQmlListProperty<ShellItem> items();

private:
    void loadElement(QQmlEngine *engine, QObject *parent, QXmlStreamReader &xml);
};

#endif
