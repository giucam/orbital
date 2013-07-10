
#include "shellui.h"

#include <QIcon>
#include <QtQml>
#include <QDebug>

static const int a = qmlRegisterType<ShellUI>("Orbital", 1, 0, "ShellUI");

ShellUI::ShellUI(QObject *p)
       : QObject(p)
{
}

ShellUI::~ShellUI()
{

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
