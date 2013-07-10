
#include <QIcon>
#include <QDebug>

#include "iconimageprovider.h"

IconImageProvider::IconImageProvider()
                 : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconImageProvider::requestPixmap(const QString &id, QSize *realSize, const QSize &requestedSize)
{
    QSize size(requestedSize);
    if (size.width() < 1) size.setWidth(1);
    if (size.height() < 1) size.setHeight(1);
    QIcon icon = QIcon::fromTheme(id);
    *realSize = size;

    return icon.pixmap(size);
}
