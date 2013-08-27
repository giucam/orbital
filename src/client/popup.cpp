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
#include <QQuickWindow>
#include <QQuickItem>
#include <QDebug>

#include "wayland-desktop-shell-client-protocol.h"
#include "popup.h"
#include "client.h"
#include "grab.h"
#include "utils.h"

static const int a = qmlRegisterType<Popup>("Orbital", 1, 0, "Popup");

Popup::Popup(QObject *p)
     : QObject(p)
     , m_window(nullptr)
     , m_parent(nullptr)
     , m_content(nullptr)
{
}

Popup::~Popup()
{
    if (m_window) {
        hide();
    }
}

bool Popup::visible() const
{
    return m_window ? m_window->isVisible() : false;
}

void Popup::setVisible(bool v)
{
    v ? show() : hide();
}

void Popup::show()
{
    if (!m_parent || !m_content || (m_window && m_window->isVisible())) {
        return;
    }

    QQuickWindow *w = m_parent->window();
    QPointF pos = m_parent->mapToScene(QPointF(0, 0));
    if (!m_window) {
        m_window = new QQuickWindow();
        m_window->setFlags(Qt::BypassWindowManagerHint);
        m_window->create();
    }
    m_window->setScreen(w->screen());

    // TODO: Better placement, maybe by adding some protocol
    m_window->setX(pos.x());
    m_window->setY(pos.y() + m_parent->height());

    m_window->setWidth(m_content->width());
    m_window->setHeight(m_content->height());

    m_window->setColor(Qt::transparent);
    m_content->setParentItem(m_window->contentItem());

    m_window->show();

    m_shsurf = Client::client()->setPopup(m_window);
    desktop_shell_surface_add_listener(m_shsurf, &m_shsurf_listener, this);

    emit visibleChanged();
}

void Popup::hide()
{
    hideEvent();
    desktop_shell_surface_destroy(m_shsurf);
}

void Popup::hideEvent()
{
    m_window->hide();
    emit visibleChanged();
}

void Popup::close(desktop_shell_surface *s)
{
    QMetaObject::invokeMethod(this, "hideEvent");
}

const desktop_shell_surface_listener Popup::m_shsurf_listener = {
    wrapInterface(&Popup::close)
};
