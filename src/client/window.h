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

#ifndef WINDOW_H
#define WINDOW_H

#include <QObject>

struct desktop_shell_window;
struct desktop_shell_window_listener;

class Window : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(States state READ state WRITE setState NOTIFY stateChanged)
public:
    enum State {
        Inactive = 0,
        Active = 1,
        Minimized = 2
    };
    Q_ENUMS(State);
    Q_DECLARE_FLAGS(States, State)
    Q_FLAGS(States)

    Window(QObject *p = nullptr);
    ~Window();
    void init(desktop_shell_window *window, int32_t state);

    inline QString title() const { return m_title; }
    void setTitle(const QString &title);

    inline States state() const { return m_state; }
    void setState(States state);

    Q_INVOKABLE bool isActive() const;
    Q_INVOKABLE bool isMinimized() const;

public slots:
    void activate();
    void minimize();
    void unminimize();

signals:
    void destroyed(Window *w);
    void titleChanged();
    void stateChanged();

private:
    void handleSetTitle(desktop_shell_window *window, const char *title);
    void handleSetState(desktop_shell_window *window, int32_t state);
    void handleRemoved(desktop_shell_window *window);

    desktop_shell_window *m_window;
    QString m_title;
    States m_state;

    static const desktop_shell_window_listener m_window_listener;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Window::States)

#endif
