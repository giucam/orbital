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

#include "window.h"
#include "wayland-desktop-shell-client-protocol.h"

static Window::States wlState2State(int32_t state)
{
    Window::States s = Window::Inactive;
    if (state & DESKTOP_SHELL_WINDOW_STATE_ACTIVE)
        s |= Window::Active;
    if (state & DESKTOP_SHELL_WINDOW_STATE_MINIMIZED)
        s |= Window::Minimized;

    return s;
}

static int32_t state2WlState(Window::States state)
{
    int32_t s = DESKTOP_SHELL_WINDOW_STATE_INACTIVE;
    if (state & Window::Active)
        s |= DESKTOP_SHELL_WINDOW_STATE_ACTIVE;
    if (state & Window::Minimized)
        s |= DESKTOP_SHELL_WINDOW_STATE_MINIMIZED;

    return s;
}

#define _this static_cast<Window *>(data)
static void set_title(void *data, desktop_shell_window *window, const char *title)
{
    _this->setTitle(title);
}

void Window::state_changed(void *data, desktop_shell_window *window, int32_t state)
{
    _this->m_state = wlState2State(state);
    emit _this->stateChanged();
}

void Window::removed(void *data, desktop_shell_window *window)
{
    emit _this->destroyed(_this);
    _this->deleteLater();
}

const desktop_shell_window_listener Window::m_window_listener = {
    set_title,
    state_changed,
    Window::removed
};

Window::Window(QObject *p)
      : QObject(p)
      , m_state(Window::Inactive)
{
}

Window::~Window()
{
    desktop_shell_window_destroy(m_window);
}

void Window::init(desktop_shell_window *w, int32_t state)
{
    m_window = w;
    desktop_shell_window_add_listener(w, &m_window_listener, this);
    m_state = wlState2State(state);
}

void Window::setTitle(const QString &t)
{
    m_title = t;
    emit titleChanged();
}

void Window::setState(States state)
{
    m_state = state;
    desktop_shell_window_set_state(m_window, state2WlState(m_state));
}

bool Window::isActive() const
{
    return m_state & Active;
}

bool Window::isMinimized() const
{
    return m_state & Minimized;
}

void Window::activate()
{
    if (!(m_state & Window::Active)) {
        setState(m_state | Window::Active);
    }
}

void Window::minimize()
{
    if (!(m_state & Window::Minimized)) {
        setState(m_state | Window::Minimized);
    }
}

void Window::unminimize()
{
    if (m_state & Window::Minimized) {
        setState(m_state & ~Window::Minimized);
    }
}
