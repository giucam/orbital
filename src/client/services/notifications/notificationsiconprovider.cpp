/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QIcon>
#include <QDebug>
#include <QPixmap>

#include "notificationsiconprovider.h"
#include "notificationsservice.h"

NotificationsIconProvider::NotificationsIconProvider(NotificationsManager *service)
                         : QQuickImageProvider(QQuickImageProvider::Pixmap)
                         , m_service(service)
{
}

QPixmap NotificationsIconProvider::requestPixmap(const QString &id, QSize *realSize, const QSize &requestedSize)
{
    QSize size(requestedSize);
    if (size.width() < 1) size.setWidth(1);
    if (size.height() < 1) size.setHeight(1);

    *realSize = size;

    int i = id.toInt();
    Notification *notification = m_service->notification(i);

    QPixmap image = notification->iconImage();
    if (!image.isNull()) {
        *realSize = image.size();
        return image;
    }

    QString iconName = notification->iconName();
    if (!iconName.isEmpty()) {
        QIcon icon = QIcon::fromTheme(iconName);
        if (!icon.isNull()) {
            return icon.pixmap(size);
        }
        qDebug("Cannot find the requested notification icon: \"%s\".", qPrintable(iconName));
    }

    return QIcon::fromTheme(QStringLiteral("dialog-information")).pixmap(size);
}
