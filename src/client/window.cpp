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
#include "utils.h"
#include "client.h"
#include "uiscreen.h"

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

void Window::handleTitle(desktop_shell_window *window, const char *title)
{
    setTitle(QString::fromUtf8(title));
}

void Window::handleIcon(desktop_shell_window *window, const char *name)
{
    m_icon = QString::fromUtf8(name);
    emit iconChanged();
}

void Window::handleState(desktop_shell_window *window, int32_t state)
{
    m_state = wlState2State(state);
    emit stateChanged();
}

void Window::handleRemoved(desktop_shell_window *window)
{
    emit destroyed(this);
    deleteLater();
}

const desktop_shell_window_listener Window::m_window_listener = {
    wrapInterface(&Window::handleTitle),
    wrapInterface(&Window::handleIcon),
    wrapInterface(&Window::handleState),
    wrapInterface(&Window::handleRemoved)
};

Window::Window(desktop_shell_window *window, pid_t pid, QObject *p)
      : QObject(p)
      , m_window(window)
      , m_pid(pid)
      , m_state(Window::Inactive)
{
    desktop_shell_window_add_listener(window, &m_window_listener, this);
}

Window::~Window()
{
    desktop_shell_window_destroy(m_window);
}

void Window::setTitle(const QString &t)
{
    m_title = t;
    emit titleChanged();
}

void Window::setState(UiScreen *screen, States state)
{
    wl_output *o = Client::client()->nativeOutput(screen->screen());
    desktop_shell_window_set_state(m_window, o, state2WlState(state));
}

bool Window::isActive() const
{
    return m_state & Active;
}

bool Window::isMinimized() const
{
    return m_state & Minimized;
}

void Window::close()
{
    desktop_shell_window_close(m_window);
}

void Window::preview(UiScreen *screen)
{
    wl_output *o = Client::client()->nativeOutput(screen->screen());
    desktop_shell_window_preview(m_window, o);
}

void Window::endPreview(UiScreen *screen)
{
    wl_output *o = Client::client()->nativeOutput(screen->screen());
    desktop_shell_window_end_preview(m_window, o);
}
