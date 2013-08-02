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

#ifndef WINDOW_H
#define WINDOW_H

#include <QObject>

struct desktop_shell_window;
struct desktop_shell_window_listener;

class Window : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
public:
    Window(QObject *p = nullptr);
    ~Window();
    void init(desktop_shell_window *window);

    inline QString title() const { return m_title; }
    void setTitle(const QString &title);

    inline bool isActive() const { return m_active; }

    void setState(int32_t state);

public slots:
    void activate();
    void minimize();

signals:
    void destroyed(Window *w);
    void titleChanged();
    void activeChanged();

private:
    desktop_shell_window *m_window;
    QString m_title;
    bool m_active;

    static void set_active(void *data, desktop_shell_window *window, int32_t activated);
    static void removed(void *data, desktop_shell_window *window);
    static const desktop_shell_window_listener m_window_listener;
};

#endif
