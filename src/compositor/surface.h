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

#include <compositor.h>

#include "interface.h"

struct wl_resource;
struct weston_surface;

namespace Orbital {

class View;
class Seat;
class Pointer;
struct Listener;

class Surface : public Object
{
    Q_OBJECT
public:
    class RoleHandler {
    public:
        RoleHandler() : surface(nullptr) {}
        virtual ~RoleHandler();
    protected:
        virtual void configure(int x, int y) = 0;
        virtual void move(Seat *seat) = 0;

    private:
        Surface *surface;
        friend Surface;
    };

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

    bool setRole(const char *roleName, wl_resource *errorResource, uint32_t errorCode);
    void setRole(const char *roleName);
    void setRoleHandler(RoleHandler *handler);
    const char *role() const;
    RoleHandler *roleHandler() const;

    void setWorkspaceMask(int mask);
    int workspaceMask() const { return m_workspaceMask; }

    void setActivable(bool activable);
    inline bool isActivable() const { return m_activable; }

    void setLabel(const QString &label);

    void ref();
    void deref();

    virtual Surface *activate();
    void move(Seat *seat);

    static Surface *fromSurface(weston_surface *s);
    static Surface *fromResource(wl_resource *resource);

signals:
    void unmapped();
    void activated(Seat *seat);
    void deactivated(Seat *seat);
    void pointerFocusEnter(Pointer *pointer, View *view);
    void pointerFocusLeave(Pointer *pointer, View *view);

private:
    static void configure(weston_surface *s, int32_t x, int32_t y);
    static void destroy(wl_listener *listener, void *data);

    weston_surface *m_surface;
    RoleHandler *m_roleHandler;
    Listener *m_listener;
    bool m_activable;
    QList<View *> m_views;
    int m_workspaceMask;
    QString m_label;

    friend View;
    friend RoleHandler;
};

}

#endif
