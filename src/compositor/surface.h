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
#include <QRect>

#include <compositor.h>

#include "interface.h"
#include "stringview.h"

struct wl_resource;
struct weston_surface;

namespace Orbital {

class View;
class ViewCreator;
class Seat;
class Pointer;
struct Listener;
class FocusScope;
class ShellSurface;

class Surface : public Object
{
    Q_OBJECT
public:
    class ActiveRegion;
private:
    struct AR {
        QRect rect;
        ActiveRegion *region;
    };

public:
    class RoleHandler {
    public:
        RoleHandler() : surface(nullptr) {}
        virtual ~RoleHandler();
    protected:
        virtual void configure(int x, int y) = 0;

    private:
        Surface *surface;
        friend Surface;
    };

    class ActiveRegion
    {
    public:
        ActiveRegion() : m_surface(nullptr) {}
        ActiveRegion(const ActiveRegion &) = delete;
        ActiveRegion(ActiveRegion &&r)
        {
            m_surface = r.m_surface;
            if (m_surface) {
                m_it = r.m_it;
                m_it->region = this;
            }
        }
        ~ActiveRegion() { if (m_surface) m_surface->m_activeRegions.erase(m_it); }

        void set(const QRect &rect) { if (m_surface) m_it->rect = rect; }

    private:
        ActiveRegion(Surface *surface, std::list<AR>::iterator it) : m_surface(surface), m_it(it)
        {
            it->region = this;
        }

        Surface *m_surface;
        std::list<Surface::AR>::iterator m_it;
        friend class Surface;
    };

    Surface(weston_surface *s, QObject *parent = nullptr);
    ~Surface();

    inline int width() const { return m_surface->width; }
    inline int height() const { return m_surface->height; }
    inline QSize size() const { return QSize(width(), height()); }
    QRect boundingBox() const;
    void map() { m_surface->is_mapped = true; }
    void unmap() { weston_surface_unmap(m_surface); }
    bool isMapped() const;
    wl_client *client() const;
    weston_surface *surface() const;
    inline const std::vector<View *> &views() const { return m_views; }
    Surface *mainSurface() const;

    void setShellSurface(ShellSurface *shsurf) { m_shsurf = shsurf; }
    ShellSurface *shellSurface() const { return m_shsurf; }

    QSize contentSize() const;
    size_t copyContent(void *data, size_t size, const QRect &rect);

    void repaint();
    void damage();

    bool setRole(const char *roleName, wl_resource *errorResource, uint32_t errorCode);
    void setRole(const char *roleName);
    void setRoleHandler(RoleHandler *handler);
    const char *role() const;
    RoleHandler *roleHandler() const;

    void setMoveHandler(const std::function<void (Seat *seat)> &handler);

    void setFocusScope(FocusScope *FocusScope);
    FocusScope *focusScope() const { return m_focusScope; }

    void setWorkspaceMask(int mask);
    int workspaceMask() const { return m_workspaceMask; }

    void setActivable(bool activable);
    inline bool isActivable() const { return m_activable; }

    ActiveRegion addActiveRegion(const QRect &rect);
    bool isActiveAt(int x, int y) const;

    void setLabel(StringView label);
    std::string label() const;

    void ref();
    void deref();

    void move(Seat *seat);

    void setViewCreator(ViewCreator *creator);
    ViewCreator *viewCreator() const { return m_viewCreator; }

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
    std::vector<View *> m_views;
    int m_workspaceMask;
    std::string m_label;
    FocusScope *m_focusScope;
    ViewCreator *m_viewCreator;
    ShellSurface *m_shsurf;
    std::function<void (Seat *seat)> m_moveHandler;
    std::list<AR> m_activeRegions;

    friend View;
    friend RoleHandler;
    friend ActiveRegion;
};

}

#endif
