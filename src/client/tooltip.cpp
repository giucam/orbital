/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <QtQml>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDebug>

#include "tooltip.h"
#include "client.h"
#include "grab.h"

static const int a = qmlRegisterType<ToolTip>("Orbital", 1, 0, "ToolTip");

int ToolTip::s_showing = 0;

ToolTip::ToolTip(QQuickItem *parent)
       : QQuickItem(parent)
       , m_window(nullptr)
       , m_content(nullptr)
       , m_showTimer(new QTimer(this))
       , m_hideTimer(new QTimer(this))
{
    m_showTimer->setInterval(500);
    m_showTimer->setSingleShot(true);
    connect(m_showTimer, &QTimer::timeout, this, &ToolTip::doShow);

    m_hideTimer->setInterval(50);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, []() { --s_showing; });
}

void ToolTip::show()
{
    if (s_showing > 0) {
        doShow();
    } else {
        m_showTimer->start();
    }
}

void ToolTip::hide()
{
    m_showTimer->stop();
    if (!m_window) {
        return;
    }

    delete m_window;
    m_window = nullptr;
    parentItem()->window()->removeEventFilter(this);

    m_hideTimer->start();
}

void ToolTip::doShow()
{
    m_hideTimer->stop();
    if (m_window || !parentItem() || !m_content) {
        return;
    }

    QQuickWindow *w = parentItem()->window();
    if (!w) {
        return;
    }
    w->installEventFilter(this);

    QPointF pos = parentItem()->mapToScene(QPointF(0, 0));
    m_window = new QQuickWindow();
    m_window->setTransientParent(w);
    m_window->setFlags(Qt::ForeignWindow);
    m_window->setScreen(w->screen());

    // TODO: Better placement, maybe by adding some protocol
    m_window->setX(pos.x());
    m_window->setY(pos.y() + parentItem()->height());

    m_window->setWidth(m_content->width());
    m_window->setHeight(m_content->height());

    m_window->setColor(Qt::transparent);
    m_content->setParentItem(m_window->contentItem());

    m_window->create();
    m_window->show();

    ++s_showing;
}

bool ToolTip::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        hide();
    }

    return QQuickItem::eventFilter(obj, event);
}
