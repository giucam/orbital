
#ifndef SHELLUI_H
#define SHELLUI_H

#include <QObject>
#include <QQmlListProperty>

#include "shellitem.h"

class ShellUI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme)
    Q_PROPERTY(QQmlListProperty<ShellItem> items READ items)
    Q_CLASSINFO("DefaultProperty", "items")

public:
    ShellUI(QObject *p = nullptr);
    ~ShellUI();

    QString iconTheme() const;
    void setIconTheme(const QString &theme);

    QQmlListProperty<ShellItem> items();
};

#endif
