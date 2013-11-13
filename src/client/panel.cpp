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

#include <QDebug>
#include <QCoreApplication>

#include "wayland-desktop-shell-client-protocol.h"
#include "panel.h"
#include "client.h"
#include "element.h"
#include "utils.h"

Panel::Panel(QScreen *screen, Element *elm)
     : QQuickWindow()
     , m_element(elm)
{
    elm->setParentItem(contentItem());
    setWidth(elm->width());
    setHeight(elm->height());
    setColor(Qt::transparent);
    setFlags(Qt::BypassWindowManagerHint);
    setScreen(screen);
    show();
    create();

    Client::client()->setInputRegion(this, elm->inputRegion());
    m_panel = Client::client()->setPanel(this, screen, (int)elm->location());
    desktop_shell_panel_add_listener(m_panel, &s_listener, this);

    connect(elm, &Element::published, this, &Panel::move);
    connect(elm, &QQuickItem::widthChanged, [this]() { setWidth(m_element->width()); });
    connect(elm, &QQuickItem::heightChanged, [this]() { setHeight(m_element->height()); });
    connect(elm, &Element::inputRegionChanged, [this]() { Client::client()->setInputRegion(this, m_element->inputRegion()); });
}

void Panel::move()
{
    desktop_shell_panel_move(m_panel);
}

void Panel::setLocation(Element::Location loc)
{
    if (loc == m_element->location()) {
        return;
    }

    m_element->setLocation(loc);
    setWidth(m_element->width());
    setHeight(m_element->height());
    Client::client()->setInputRegion(this, m_element->inputRegion());
    desktop_shell_panel_set_position(m_panel, (uint32_t)loc);
}

void Panel::locationChanged(desktop_shell_panel *panel, uint32_t loc)
{
    setLocation((Element::Location)loc);
}

const desktop_shell_panel_listener Panel::s_listener = {
    wrapInterface(&Panel::locationChanged)
};

