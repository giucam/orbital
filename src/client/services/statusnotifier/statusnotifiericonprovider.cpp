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

#include "statusnotifiericonprovider.h"
#include "statusnotifieritem.h"
#include "statusnotifierservice.h"

StatusNotifierIconProvider::StatusNotifierIconProvider(StatusNotifierManager *service)
                          : QQuickImageProvider(QQuickImageProvider::Pixmap)
                          , m_service(service)
{
}

static QPixmap getPixmap(StatusNotifierItem *item, const QSize &s, const QString &type)
{
    if (type == QStringLiteral("attentionIcon")) {
        QPixmap pix = item->attentionIconPixmap(s);
        if (pix.isNull()) {
            QString name = item->attentionIconName();
            if (!name.isEmpty()) {
                QIcon icon = QIcon::fromTheme(name);
                pix = icon.pixmap(s);
            }
        }
        return pix;
    }

    QPixmap pix = item->iconPixmap(s);
    if (pix.isNull()) {
        QString name = item->iconName();
        if (!name.isEmpty()) {
            QIcon icon = QIcon::fromTheme(name);
            pix = icon.pixmap(s);
        }
    }
    return pix;
}

QPixmap StatusNotifierIconProvider::requestPixmap(const QString &id, QSize *realSize, const QSize &requestedSize)
{
    QSize size(requestedSize);
    if (size.width() < 1) size.setWidth(1);
    if (size.height() < 1) size.setHeight(1);

    *realSize = size;

#define FAIL \
    qWarning("StatusNotifierIconProvider: cannot load icon \"%s\".", qPrintable(id)); \
    return QIcon::fromTheme(QStringLiteral("image-missing")).pixmap(size);

    QStringList l = id.split('/');
    if (l.size() != 2) {
        FAIL
    }

    QString service = l.first();
    QString name = l.at(1);

    StatusNotifierItem *item = m_service->item(service);
    if (item) {
        QPixmap pix = getPixmap(item, size, name);
        if (!pix.isNull())
            return pix;
    }

    FAIL
}
