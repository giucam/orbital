
#include <QtQml>
#include <QDebug>

#include "shellitem.h"

static const int a = qmlRegisterType<ShellItem>("Orbital", 1, 0, "ShellItem");

ShellItem::ShellItem(QWindow *p)
         : QQuickWindow(p)
         , m_type(None)
{
}
