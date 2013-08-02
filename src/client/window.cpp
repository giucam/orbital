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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>

#include "window.h"
#include "wayland-desktop-shell-client-protocol.h"

#define _this static_cast<Window *>(data)
static void set_title(void *data, desktop_shell_window *window, const char *title)
{
    _this->setTitle(title);
}

void Window::set_active(void *data, desktop_shell_window *window, int32_t activated)
{
    _this->m_active = activated;
    emit _this->activeChanged();
}

void Window::removed(void *data, desktop_shell_window *window)
{
    emit _this->destroyed(_this);
    _this->deleteLater();
}

const desktop_shell_window_listener Window::m_window_listener = {
    set_title,
    Window::set_active,
    Window::removed
};

Window::Window(QObject *p)
      : QObject(p)
      , m_active(false)
{
}

Window::~Window()
{
    desktop_shell_window_destroy(m_window);
}

void Window::init(desktop_shell_window *w)
{
    m_window = w;
    desktop_shell_window_add_listener(w, &m_window_listener, this);
}

void Window::setTitle(const QString &t)
{
    m_title = t;
    emit titleChanged();
}

void Window::setState(int32_t state)
{
    if (state == DESKTOP_SHELL_WINDOW_STATE_ACTIVE) {
        m_active = true;
    } else {
        m_active = false;
    }
}

void Window::activate()
{
    desktop_shell_window_activate(m_window);
}

void Window::minimize()
{
    desktop_shell_window_minimize(m_window);
}
