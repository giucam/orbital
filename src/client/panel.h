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

#ifndef PANEL_H
#define PANEL_H

#include <QQuickWindow>

#include "element.h"

class QScreen;

struct desktop_shell_panel;
struct desktop_shell_panel_listener;

class Element;

class Panel : public QQuickWindow
{
    Q_OBJECT
public:
    Panel(QScreen *screen, Element *elm);
    ~Panel();

    void move();
    void setLocation(Element::Location loc);

private:
    void locationChanged(desktop_shell_panel *panel, uint32_t loc);

    desktop_shell_panel *m_panel;
    Element *m_element;

    static const desktop_shell_panel_listener s_listener;
};

#endif
