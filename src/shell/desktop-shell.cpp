/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>

#include <wayland-server.h>

#include <weston/compositor.h>
#include <weston/matrix.h>
// #include <weston/config-parser.h>

#include "desktop-shell.h"
#include "wayland-desktop-shell-server-protocol.h"
#include "shellsurface.h"

#include "scaleeffect.h"
#include "griddesktops.h"
#include "fademovingeffect.h"
#include "zoomeffect.h"
#include "inoutsurfaceeffect.h"
#include "inputpanel.h"
#include "shellseat.h"
#include "workspace.h"
#include "minimizeeffect.h"

DesktopShell::DesktopShell(struct weston_compositor *ec)
            : Shell(ec)
{
}

void DesktopShell::init()
{
    Shell::init();

    if (!wl_global_create(compositor()->wl_display, &desktop_shell_interface, 1, this,
        [](struct wl_client *client, void *data, uint32_t version, uint32_t id) { static_cast<DesktopShell *>(data)->bind(client, version, id); }))
        return;

    weston_compositor_add_button_binding(compositor(), BTN_LEFT, MODIFIER_SUPER,
                                         [](struct weston_seat *seat, uint32_t time, uint32_t button, void *data) {
                                             static_cast<DesktopShell *>(data)->moveBinding(seat, time, button);
                                         }, this);

    weston_compositor_add_button_binding(compositor(), BTN_MIDDLE, MODIFIER_SUPER,
                                         [](struct weston_seat *seat, uint32_t time, uint32_t button, void *data) {
                                             static_cast<DesktopShell *>(data)->resizeBinding(seat, time, button);
                                         }, this);

    new ScaleEffect(this);
    new GridDesktops(this);
    new FadeMovingEffect(this);
    new ZoomEffect(this);
    new InOutSurfaceEffect(this);
    new MinimizeEffect(this);

    m_inputPanel = new InputPanel(compositor()->wl_display);
}

void DesktopShell::setGrabCursor(uint32_t cursor)
{
    desktop_shell_send_grab_cursor(m_child.desktop_shell, cursor);
}

struct BusyGrab : public ShellGrab {
    void focus() override
    {
        wl_fixed_t sx, sy;
        weston_surface *es = weston_compositor_pick_surface(pointer()->seat->compositor, pointer()->x, pointer()->y, &sx, &sy);

        if (surface->weston_surface() != es) {
            delete this;
        }
    }
    void button(uint32_t time, uint32_t button, uint32_t state) override
    {
        weston_seat *seat = pointer()->seat;

        if (surface && button == BTN_LEFT && state) {
            ShellSeat::shellSeat(seat)->activate(surface);
            surface->dragMove(seat);
        } else if (surface && button == BTN_RIGHT && state) {
            ShellSeat::shellSeat(seat)->activate(surface);
//         surface_rotate(grab->surface, &seat->seat);
        }
    }

    ShellSurface *surface;
};

void DesktopShell::setBusyCursor(ShellSurface *surface, struct weston_seat *seat)
{
    BusyGrab *grab = new BusyGrab;
    if (!grab && grab->pointer())
        return;

    grab->surface = surface;
    startGrab(grab, seat, DESKTOP_SHELL_CURSOR_BUSY);
}

void DesktopShell::endBusyCursor(struct weston_seat *seat)
{
    ShellGrab *grab = ShellGrab::fromGrab(seat->pointer->grab);

    if (dynamic_cast<BusyGrab *>(grab)) {
        delete grab;
    }
}

void DesktopShell::sendInitEvents()
{
    for (uint i = 0; i < numWorkspaces(); ++i) {
        workspace(i)->init(m_child.client);
        workspaceAdded(workspace(i));
    }

    for (ShellSurface *shsurf: surfaces()) {
        shsurf->advertize();
    }
}

void DesktopShell::workspaceAdded(Workspace *ws)
{
    desktop_shell_send_workspace_added(m_child.desktop_shell, ws->resource(), ws->active());
}

void DesktopShell::bind(struct wl_client *client, uint32_t version, uint32_t id)
{
    struct wl_resource *resource = wl_resource_create(client, &desktop_shell_interface, version, id);

    if (client == m_child.client) {
        wl_resource_set_implementation(resource, &m_desktop_shell_implementation, this,
                                       [](struct wl_resource *resource) { static_cast<DesktopShell *>(wl_resource_get_user_data(resource))->unbind(resource); });
        m_child.desktop_shell = resource;

        sendInitEvents();
        desktop_shell_send_load(resource);
        return;
    }

    wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "permission to bind desktop_shell denied");
    wl_resource_destroy(resource);
}

void DesktopShell::unbind(struct wl_resource *resource)
{
    m_child.desktop_shell = nullptr;
}

void DesktopShell::moveBinding(struct weston_seat *seat, uint32_t time, uint32_t button)
{
    struct weston_surface *surface = seat->pointer->focus;
    if (!surface) {
        return;
    }

    ShellSurface *shsurf = getShellSurface(surface);
    if (!shsurf || shsurf->type() == ShellSurface::Type::Fullscreen || shsurf->type() == ShellSurface::Type::Maximized) {
        return;
    }

    shsurf = shsurf->topLevelParent();
    if (shsurf) {
        shsurf->dragMove(seat);
    }
}

void DesktopShell::resizeBinding(weston_seat *seat, uint32_t time, uint32_t button)
{
    weston_surface *surface = weston_surface_get_main_surface(seat->pointer->focus);
    if (!surface) {
        return;
    }

    ShellSurface *shsurf = getShellSurface(surface);
    if (!shsurf || shsurf->type() == ShellSurface::Type::Fullscreen || shsurf->type() == ShellSurface::Type::Maximized) {
        return;
    }

    shsurf = shsurf->topLevelParent();
    if (shsurf) {
        int32_t x, y;
        weston_surface_from_global(surface, wl_fixed_to_int(seat->pointer->grab_x), wl_fixed_to_int(seat->pointer->grab_y), &x, &y);

        pixman_box32_t *bbox = pixman_region32_extents(&surface->input);

        uint32_t edges = 0;
        int32_t w = surface->geometry.width / 3;
        if (w > 20) w = 20;
        w += bbox->x1;

        if (x < w)
            edges |= WL_SHELL_SURFACE_RESIZE_LEFT;
        else if (x < surface->geometry.width - w)
            edges |= 0;
        else
            edges |= WL_SHELL_SURFACE_RESIZE_RIGHT;

        int32_t h = surface->geometry.height / 3;
        if (h > 20) h = 20;
        h += bbox->y1;
        if (y < h)
            edges |= WL_SHELL_SURFACE_RESIZE_TOP;
        else if (y < surface->geometry.height - h)
            edges |= 0;
        else
            edges |= WL_SHELL_SURFACE_RESIZE_BOTTOM;

        shsurf->dragResize(seat, edges);
    }
}

void DesktopShell::setBackground(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource,
                                 struct wl_resource *surface_resource)
{
    struct weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));

    setBackgroundSurface(surface, static_cast<weston_output *>(wl_resource_get_user_data(output_resource)));

    desktop_shell_send_configure(resource, 0,
                                 surface_resource,
                                 surface->output->width,
                                 surface->output->height);
}

void DesktopShell::setPanel(wl_client *client, wl_resource *resource, wl_resource *output_resource, wl_resource *surface_resource, uint32_t pos)
{
    struct weston_surface *surface = static_cast<struct weston_surface *>(wl_resource_get_user_data(surface_resource));

    addPanelSurface(surface, static_cast<weston_output *>(wl_resource_get_user_data(output_resource)), (Shell::PanelPosition)pos);
    desktop_shell_send_configure(resource, 0, surface_resource, surface->output->width, surface->output->height);
}

void DesktopShell::setLockSurface(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface_resource)
{
//     struct desktop_shell *shell = resource->data;
//     struct weston_surface *surface = surface_resource->data;
//
//     shell->prepare_event_sent = false;
//
//     if (!shell->locked)
//         return;
//
//     shell->lock_surface = surface;
//
//     shell->lock_surface_listener.notify = handle_lock_surface_destroy;
//     wl_signal_add(&surface_resource->destroy_signal,
//                   &shell->lock_surface_listener);
//
//     surface->configure = lock_surface_configure;
//     surface->configure_private = shell;
}

class PopupGrab : public ShellGrab {
public:
    weston_surface *surface;
    wl_resource *shsurfResource;
    bool inside;
    uint32_t creationTime;

    void focus() override
    {
        wl_fixed_t sx, sy;
        weston_surface *es = weston_compositor_pick_surface(pointer()->seat->compositor, pointer()->x, pointer()->y, &sx, &sy);

        inside = es == surface;
        if (inside)
            weston_pointer_set_focus(pointer(), surface, sx, sy);
    }
    void motion(uint32_t time) override
    {
        wl_resource *resource;
        wl_resource_for_each(resource, &pointer()->focus_resource_list) {
            wl_fixed_t sx, sy;
            weston_surface_from_global_fixed(pointer()->focus, pointer()->x, pointer()->y, &sx, &sy);
            wl_pointer_send_motion(resource, time, sx, sy);
        }
    }
    void button(uint32_t time, uint32_t button, uint32_t state) override
    {
        wl_resource *resource;
        wl_resource_for_each(resource, &pointer()->focus_resource_list) {
            struct wl_display *display = wl_client_get_display(wl_resource_get_client(resource));
            uint32_t serial = wl_display_get_serial(display);
            wl_pointer_send_button(resource, serial, time, button, state);
        }

        // this time check is to ensure the window doesn't get shown and hidden very fast, mainly because
        // there is a bug in QQuickWindow, which hangs up the process.
        if (!inside && state == WL_POINTER_BUTTON_STATE_RELEASED && time - creationTime > 500) {
            desktop_shell_surface_send_popup_close(shsurfResource);
            wl_resource_destroy(shsurfResource);
        }
    }
};

static void popupGrabDestroyed(wl_resource *res)
{
    delete static_cast<PopupGrab *>(wl_resource_get_user_data(res));
}

void DesktopShell::setPopup(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *output_resource, wl_resource *surface_resource, int x, int y)
{
    struct weston_surface *surface = static_cast<struct weston_surface *>(wl_resource_get_user_data(surface_resource));

    if (!surface->configure) {
        // FIXME: change/rename this function
        addPanelSurface(surface, static_cast<weston_output *>(wl_resource_get_user_data(output_resource)), Shell::PanelPosition::Top);
    }
    weston_surface_set_position(surface, x, y);

    PopupGrab *grab = new PopupGrab;
    if (!grab)
        return;

    grab->shsurfResource = wl_resource_create(client, &desktop_shell_surface_interface, wl_resource_get_version(resource), id);
    wl_resource_set_destructor(grab->shsurfResource, popupGrabDestroyed);
    wl_resource_set_user_data(grab->shsurfResource, grab);

    weston_seat *seat = container_of(compositor()->seat_list.next, weston_seat, link);
    grab->surface = surface;
    grab->creationTime = seat->pointer->grab_time;

    wl_fixed_t sx, sy;
    weston_surface_from_global_fixed(surface, seat->pointer->x, seat->pointer->y, &sx, &sy);
    weston_pointer_set_focus(seat->pointer, surface, sx, sy);

    startGrab(grab, seat);
}

void DesktopShell::unlock(struct wl_client *client, struct wl_resource *resource)
{
//     struct desktop_shell *shell = resource->data;
//
//     shell->prepare_event_sent = false;
//
//     if (shell->locked)
//         resume_desktop(shell);
}

void DesktopShell::setGrabSurface(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface_resource)
{
    this->Shell::setGrabSurface(static_cast<struct weston_surface *>(wl_resource_get_user_data(surface_resource)));
}

void DesktopShell::desktopReady(struct wl_client *client, struct wl_resource *resource)
{
    fadeSplash();
}

void DesktopShell::addKeyBinding(struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t key, uint32_t modifiers)
{
    wl_resource *res = wl_resource_create(client, &desktop_shell_binding_interface, wl_resource_get_version(resource), id);
    wl_resource_set_implementation(res, nullptr, res, [](wl_resource *) {});

    weston_compositor_add_key_binding(compositor(), key, (weston_keyboard_modifier)modifiers,
                                         [](struct weston_seat *seat, uint32_t time, uint32_t key, void *data) {

                                             desktop_shell_binding_send_triggered(static_cast<wl_resource *>(data));
                                         }, res);
}

void DesktopShell::addOverlay(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource)
{
    struct weston_surface *surface = static_cast<weston_surface *>(wl_resource_get_user_data(surface_resource));

    addOverlaySurface(surface, static_cast<weston_output *>(wl_resource_get_user_data(output_resource)));
    desktop_shell_send_configure(resource, 0, surface_resource, surface->output->width, surface->output->height);
    pixman_region32_fini(&surface->pending.input);
    pixman_region32_init_rect(&surface->pending.input, 0, 0, 0, 0);
}

void DesktopShell::addWorkspace(wl_client *client, wl_resource *resource)
{
    Workspace *ws = new Workspace(this, numWorkspaces());
    ws->init(client);
    Shell::addWorkspace(ws);
    workspaceAdded(ws);
}

void DesktopShell::selectWorkspace(wl_client *client, wl_resource *resource, wl_resource *workspace_resource)
{
    Shell::selectWorkspace(Workspace::fromResource(workspace_resource)->number());
}

class ClientGrab : public ShellGrab {
public:
    void focus() override
    {
        wl_fixed_t sx, sy;
        weston_surface *surface = weston_compositor_pick_surface(pointer()->seat->compositor, pointer()->x, pointer()->y, &sx, &sy);
        if (surfFocus != surface) {
            surfFocus = surface;
            desktop_shell_grab_send_focus(resource, surface->resource, sx, sy);
        }
    }
    void motion(uint32_t time) override
    {
        wl_fixed_t sx = pointer()->x;
        wl_fixed_t sy = pointer()->y;
        if (surfFocus) {
            weston_surface_from_global_fixed(surfFocus, pointer()->x, pointer()->y, &sx, &sy);
        }

        desktop_shell_grab_send_motion(resource, time, sx, sy);
    }
    void button(uint32_t time, uint32_t button, uint32_t state) override
    {
        // Send the event to the application as normal if the mouse was pressed initially.
        // The application has to know the button was released, otherwise its internal state
        // will be inconsistent with the physical button state.
        // Eat the other events, as the app doesn't need to know them.
        // NOTE: this works only if there is only 1 button pressed initially. i can know how many button
        // are pressed but weston currently has no API to determine which ones they are.
        wl_resource *resource;
        wl_resource_for_each(resource, &pointer()->focus_resource_list) {
            if (pressed && button == pointer()->grab_button) {
                wl_display *display = wl_client_get_display(wl_resource_get_client(resource));
                uint32_t serial = wl_display_next_serial(display);
                wl_pointer_send_button(resource, serial, time, button, state);
                pressed = false;
            }
        }

        desktop_shell_grab_send_button(this->resource, time, button, state);
    }

    wl_resource *resource;
    weston_surface *surfFocus;
    bool pressed;
};

static void clientGrabDestroyed(wl_resource *res)
{
    delete static_cast<ClientGrab *>(wl_resource_get_user_data(res));
}

void client_grab_end(wl_client *client, wl_resource *resource)
{
    ClientGrab *cg = static_cast<ClientGrab *>(wl_resource_get_user_data(resource));
    weston_output_schedule_repaint(cg->pointer()->focus->output);
    cg->end();
}

static const struct desktop_shell_grab_interface desktop_shell_grab_implementation = {
    client_grab_end
};

void DesktopShell::createGrab(wl_client *client, wl_resource *resource, uint32_t id)
{
    wl_resource *res = wl_resource_create(client, &desktop_shell_grab_interface, wl_resource_get_version(resource), id);

    ClientGrab *grab = new ClientGrab;
    wl_resource_set_implementation(res, &desktop_shell_grab_implementation, grab, clientGrabDestroyed);

    if (!grab)
        return;

    weston_seat *seat = container_of(compositor()->seat_list.next, weston_seat, link);
    grab->resource = res;
    grab->pressed = seat->pointer->button_count > 0;

    ShellSeat::shellSeat(seat)->endPopupGrab();

    wl_fixed_t sx, sy;
    struct weston_surface *surface = weston_compositor_pick_surface(compositor(),
                                                                    seat->pointer->x, seat->pointer->y,
                                                                    &sx, &sy);
    startGrab(grab, seat);

    weston_pointer_set_focus(seat->pointer, surface, sx, sy);
    grab->surfFocus = surface;
    desktop_shell_grab_send_focus(grab->resource, surface->resource, sx, sy);
}

void DesktopShell::quit(wl_client *client, wl_resource *resource)
{
    Shell::quit();
}

const struct desktop_shell_interface DesktopShell::m_desktop_shell_implementation = {
    wrapInterface(&DesktopShell::setBackground),
    wrapInterface(&DesktopShell::setPanel),
    wrapInterface(&DesktopShell::setLockSurface),
    wrapInterface(&DesktopShell::setPopup),
    wrapInterface(&DesktopShell::unlock),
    wrapInterface(&DesktopShell::setGrabSurface),
    wrapInterface(&DesktopShell::desktopReady),
    wrapInterface(&DesktopShell::addKeyBinding),
    wrapInterface(&DesktopShell::addOverlay),
    wrapInterface(&Shell::minimizeWindows),
    wrapInterface(&Shell::restoreWindows),
    wrapInterface(&DesktopShell::createGrab),
    wrapInterface(&DesktopShell::addWorkspace),
    wrapInterface(&DesktopShell::selectWorkspace),
    wrapInterface(&DesktopShell::quit)
};
