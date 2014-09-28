/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_SURFACE_H
#define ORBITAL_SURFACE_H

#include <functional>

#include <QSize>

#include <weston/compositor.h>

#include "interface.h"

struct wl_resource;
struct weston_surface;

namespace Orbital {

class View;
class Seat;
struct Listener;

class Surface : public Object
{
    Q_OBJECT
public:
    typedef std::function<void (int, int)> ConfigureHandler;
    struct Role { };

    Surface(weston_surface *s, QObject *parent = nullptr);
    ~Surface();

    inline int width() const { return m_surface->width; }
    inline int height() const { return m_surface->height; }
    inline QSize size() const { return QSize(width(), height()); }
    bool isMapped() const;
    wl_client *client() const;
    weston_surface *surface() const;
    inline QList<View *> views() const { return m_views; }

    void repaint();
    void damage();
    void setRole(Role *role, const ConfigureHandler &handler);
    Role *role() const;
    ConfigureHandler configureHandler() const;

    void setActivable(bool activable);
    inline bool isActivable() const { return m_activable; }

    void ref();
    void deref();

    virtual Surface *activate(Seat *seat);
    virtual void move(Seat *seat) {}

    static Surface *fromSurface(weston_surface *s);
    static Surface *fromResource(wl_resource *resource);

signals:
    void unmapped();
    void activated(Seat *seat);
    void deactivated(Seat *seat);

private:
    static void configure(weston_surface *s, int32_t x, int32_t y);

    weston_surface *m_surface;
    Role *m_role;
    ConfigureHandler m_configureHandler;
    Listener *m_listener;
    bool m_activable;
    QList<View *> m_views;

    friend View;
};

}

#endif
