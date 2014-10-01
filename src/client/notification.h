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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QObject>

class QQuickWindow;
class QQuickItem;

struct notification_surface;

class Notification : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool inactive READ inactive WRITE setInactive)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem)
    Q_CLASSINFO("DefaultProperty", "contentItem")
public:
    explicit Notification(QObject *p = nullptr);
    ~Notification();

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);
    bool inactive() const;
    void setInactive(bool inactive);

private:
    void resetWidth();
    void resetHeight();

    QQuickWindow *m_window;
    QQuickItem *m_contentItem;
    bool m_inactive;
    notification_surface *m_surface;
};

#endif
