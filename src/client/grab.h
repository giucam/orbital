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

#ifndef GRAB_H
#define GRAB_H

#include <QObject>

#include <wayland-client.h>

struct desktop_shell_grab;
struct desktop_shell_grab_listener;

class Grab : public QObject
{
    Q_OBJECT
public:
    Grab(desktop_shell_grab *grab);
    ~Grab();

public slots:
    void end();

signals:
    void focus(wl_surface *surface, int x, int y);
    void motion(uint32_t time, int x, int y);
    void button(uint32_t time, uint32_t button, uint32_t state);

private:
    void handleFocus(desktop_shell_grab *grab, wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
    void handleMotion(desktop_shell_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    void handleButton(desktop_shell_grab *grab, uint32_t time , uint32_t button, uint32_t state);
    static const desktop_shell_grab_listener s_desktop_shell_grab_listener;

    desktop_shell_grab *m_grab;
};

#endif
