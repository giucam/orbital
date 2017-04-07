/*
 * Copyright 2016 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include "wdesktop.h"
#include "../compositor.h"
#include "../shell.h"
#include "../surface.h"
#include "../shellsurface.h"
#include "../view.h"
#include "../seat.h"
#include "../surface.h"

namespace Orbital {

template<class F>
struct ApiWrapper {};

template<class R, class T, class... Args>
struct ApiWrapper<R (T::*)(Args...)>
{
    template<R (T::*func)(Args...)>
    static constexpr void forward(Args... args, void *ud) {
        (static_cast<T *>(ud)->*func)(args...);
    }
};

WDesktop::WDesktop(Shell *shell, Compositor *c)
        : Interface(shell)
        , m_compositor(c)
        , m_shell(shell)
{
#define API(func) ApiWrapper<decltype(&std::remove_pointer_t<decltype(this)>::func)>::forward<&std::remove_pointer_t<decltype(this)>::func>
    static const weston_desktop_api wdesktopApi = {
        .struct_size = sizeof(weston_desktop_api),
        .ping_timeout = API(pingTimeout),
        .pong = API(pong),
        .surface_added = API(surfaceAdded),
        .surface_removed = API(surfaceRemoved),
        .committed = API(committed),
        .show_window_menu = API(showWindowMenu),
        .set_parent = API(setParent),
        .move = API(move),
        .resize = API(resize),
        .fullscreen_requested = API(fullscreenRequested),
        .maximized_requested = API(maximizedRequested),
        .minimized_requested = API(minimizedRequested),
        .set_xwayland_position = API(setXWaylandPosition),
    };
#undef API

    m_wdesktop = weston_desktop_create(c->compositor(), &wdesktopApi, this);

    auto connectSeat = [this](Seat *s) {
        connect(s, &Seat::pointerFocus, this, &WDesktop::pointerFocus);
    };
    for (auto seat: c->seats()) {
        connectSeat(seat);
    }
    connect(c, &Compositor::seatCreated, connectSeat);
}

WDesktop::~WDesktop()
{
    weston_desktop_destroy(m_wdesktop);
}

class DesktopSurface
{
public:
    weston_desktop_surface *m_wds;
    ShellSurface *shsurf;
    std::vector<QMetaObject::Connection> connections;
    bool maximized, fullscreen;

    DesktopSurface(weston_desktop_surface *wds)
        : m_wds(wds), maximized(false), fullscreen(false) {}

    void setSize(int w, int h)
    {
        weston_desktop_surface_set_size(m_wds, w, h);
    }
    QRect geometry() const
    {
        auto geom = weston_desktop_surface_get_geometry(m_wds);
        return QRect(geom.x, geom.y, geom.width, geom.height);
    }

    static DesktopSurface *get(weston_desktop_surface *wds)
    {
        return static_cast<DesktopSurface *>(weston_desktop_surface_get_user_data(wds));
    }
    static ShellSurface *getShSurf(weston_desktop_surface *wds)
    {
        auto ds = get(wds);
        return ds ? ds->shsurf : nullptr;
    }
};

void WDesktop::pingTimeout(weston_desktop_client *client)
{
    weston_desktop_client_for_each_surface(client, [](weston_desktop_surface *surface, void *ud) {
        auto shsurf = DesktopSurface::getShSurf(surface);
        auto client = weston_desktop_surface_get_client(surface);
        shsurf->setIsResponsive(false);

        auto wdesktop = static_cast<WDesktop *>(ud);
        for (auto seat: wdesktop->m_compositor->seats()) {
            auto view = seat->pointer()->focus();
            if (!view) {
                continue;
            }

            auto wds = weston_surface_get_desktop_surface(view->surface()->surface());
            if (!wds) {
                continue;
            }
            auto shsurf = DesktopSurface::getShSurf(wds);
            if (!shsurf) {
                continue;
            }

            if (client == weston_desktop_surface_get_client(wds)) {
                shsurf->setBusyCursor(seat->pointer());
            }
        }
    }, this);
}

void WDesktop::pong(weston_desktop_client *client)
{
    weston_desktop_client_for_each_surface(client, [](weston_desktop_surface *surface, void *ud) {
        auto shsurf = DesktopSurface::getShSurf(surface);
        shsurf->setIsResponsive(true);
    }, this);
}

void WDesktop::surfaceAdded(weston_desktop_surface *wds)
{
    Surface *surface = Surface::fromSurface(weston_desktop_surface_get_surface(wds));

    static class Creator : public ViewCreator {
    public:
        weston_view *create(Surface *surface) override
        {
            weston_desktop_surface *ds = weston_surface_get_desktop_surface(surface->surface());
            return weston_desktop_surface_create_view(ds);
        }
        void destroy(View *view, weston_view *wv) override
        {
            weston_desktop_surface_unlink_view(wv);
            weston_view_destroy(wv);
        }

        WDesktop *desktop;
    } creator = {};

    creator.desktop = this;

    surface->setViewCreator(&creator);
    auto *ds = new DesktopSurface(wds);
    ds->shsurf = m_shell->createShellSurface(surface, *ds);

    ds->shsurf->setToplevel();

    weston_desktop_surface_set_user_data(wds, ds);

    ds->connections.push_back(connect(surface, &Surface::activated, [wds](Seat *seat) {
        weston_desktop_surface_set_activated(wds, true);
    }));
    ds->connections.push_back(connect(surface, &Surface::deactivated, [wds](Seat *seat) {
        weston_desktop_surface_set_activated(wds, false);
    }));
}

void WDesktop::surfaceRemoved(weston_desktop_surface *surface)
{
    auto ds = DesktopSurface::get(surface);
    ds->shsurf->setHandler({});
    ds->shsurf->surface()->setViewCreator(nullptr);

    for (auto &conn: ds->connections) {
        disconnect(conn);
    }

    delete ds;
}

void WDesktop::committed(weston_desktop_surface *surface, int32_t sx, int32_t sy)
{
    auto ds = DesktopSurface::get(surface);

    bool maximized = weston_desktop_surface_get_maximized(surface);
    bool fullscreen = weston_desktop_surface_get_fullscreen(surface);

    if ((!maximized && ds->maximized) || (!fullscreen && ds->fullscreen)) {
        ds->shsurf->setToplevel();
    }

    ds->maximized = maximized;
    ds->fullscreen = fullscreen;

    ds->shsurf->setTitle(weston_desktop_surface_get_title(surface));
    ds->shsurf->setAppId(weston_desktop_surface_get_app_id(surface));
    ds->shsurf->setPid(weston_desktop_surface_get_pid(surface));

    ds->shsurf->committed(sx, sy);
}

void WDesktop::showWindowMenu(weston_desktop_surface *surface, weston_seat *seat, int32_t x, int32_t y)
{
}

void WDesktop::setParent(weston_desktop_surface *surface, weston_desktop_surface *parent)
{
    auto shsurf = DesktopSurface::getShSurf(surface);
    auto p = parent ? DesktopSurface::getShSurf(parent) : nullptr;

    if (!shsurf) {
        return;
    }

    if (p) {
        shsurf->setParent(p->surface(), 0, 0, 0);
    } else {
        shsurf->setToplevel();
    }
}

void WDesktop::move(weston_desktop_surface *surface, weston_seat *ws, uint32_t serial)
{
    auto shsurf = DesktopSurface::getShSurf(surface);
    auto seat = Seat::fromSeat(ws);

    if (shsurf) {
        shsurf->move(seat);
    }
}

void WDesktop::resize(weston_desktop_surface *surface, weston_seat *ws, uint32_t serial, weston_desktop_surface_edge edges)
{
    auto shsurf = DesktopSurface::getShSurf(surface);
    auto seat = Seat::fromSeat(ws);

    if (shsurf) {
        shsurf->resize(seat, (ShellSurface::Edges)edges);
    }
}

void WDesktop::fullscreenRequested(weston_desktop_surface *surface, bool fullscreen, weston_output *output)
{
    auto shsurf = DesktopSurface::getShSurf(surface);
    if (!shsurf) {
        return;
    }

    if (fullscreen) {
        shsurf->setFullscreen();
    }
    weston_desktop_surface_set_fullscreen(surface, fullscreen);
}

void WDesktop::maximizedRequested(weston_desktop_surface *surface, bool maximized)
{
    auto shsurf = DesktopSurface::getShSurf(surface);
    if (!shsurf) {
        return;
    }

    if (maximized) {
        shsurf->setMaximized();
    }
    weston_desktop_surface_set_maximized(surface, maximized);
}

void WDesktop::minimizedRequested(weston_desktop_surface *surface)
{
    auto shsurf = DesktopSurface::getShSurf(surface);
    if (shsurf) {
        shsurf->minimize();
    }
}

void WDesktop::setXWaylandPosition(weston_desktop_surface *surface, int32_t x, int32_t y)
{
}

void WDesktop::pointerFocus(Pointer *pointer)
{
    auto view = pointer->focus();
    if (!view) {
        return;
    }

    auto wds = weston_surface_get_desktop_surface(view->surface()->surface());
    if (!wds) {
        return;
    }
    auto shsurf = DesktopSurface::getShSurf(wds);
    if (!shsurf) {
        return;
    }

    if (shsurf->isResponsive()) {
        auto client = weston_desktop_surface_get_client(wds);
        weston_desktop_client_ping(client);
    } else {
        shsurf->setBusyCursor(pointer);
    }
}


}
