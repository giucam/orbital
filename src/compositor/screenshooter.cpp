/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <compositor.h>

#include "screenshooter.h"
#include "compositor.h"
#include "shell.h"
#include "output.h"
#include "utils.h"
#include "seat.h"
#include "view.h"
#include "surface.h"
#include "wayland-screenshooter-server-protocol.h"

namespace Orbital {

Screenshooter::Screenshooter(Shell *s)
             : Interface(s)
             , RestrictedGlobal(s->compositor(), &orbital_screenshooter_interface, 1)
             , m_compositor(s->compositor())
{
}

void Screenshooter::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &orbital_screenshooter_interface, version, id);

    static const struct orbital_screenshooter_interface implementation = {
        wrapInterface(&Screenshooter::shoot),
        wrapInterface(&Screenshooter::shootSurface),
    };

    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void Screenshooter::shoot(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *outputResource, wl_resource *bufferResource)
{
    weston_output *output = static_cast<weston_output *>(wl_resource_get_user_data(outputResource));
    weston_buffer *buffer = weston_buffer_from_resource(bufferResource);

    if (!buffer) {
        wl_resource_post_no_memory(resource);
        return;
    }

    struct Screenshot
    {
        static void done(void *data, weston_screenshooter_outcome outcome)
        {
            Screenshot *ss = static_cast<Screenshot *>(data);

            switch (outcome) {
                case WESTON_SCREENSHOOTER_SUCCESS:
                    orbital_screenshot_send_done(ss->resource);
                    break;
                case WESTON_SCREENSHOOTER_NO_MEMORY:
                    wl_resource_post_no_memory(ss->resource);
                    break;
                case WESTON_SCREENSHOOTER_BAD_BUFFER:
                    orbital_screenshot_send_failed(ss->resource);
                    break;
            }

            wl_resource_destroy(ss->resource);
            delete ss;
        }

        wl_resource *resource;
    };

    Screenshot *ss = new Screenshot;
    ss->resource = wl_resource_create(client, &orbital_screenshot_interface, 1, id);
    if (!ss->resource) {
        wl_resource_post_no_memory(resource);
        return;
    }

    weston_screenshooter_shoot(output, buffer, Screenshot::done, ss);
}

void Screenshooter::shootSurface(wl_client *client, wl_resource *resource, uint32_t id)
{
    wl_resource *res = wl_resource_create(client, &orbital_surface_screenshot_interface, 1, id);
    class SurfaceScreenshot : public PointerGrab
    {
    public:
        SurfaceScreenshot(Compositor *c, wl_resource *res)
            : m_compositor(c)
            , m_resource(res)
            , m_surface(nullptr)
        {
            static const struct orbital_surface_screenshot_interface implementation = {
                wrapInterface(&SurfaceScreenshot::shoot),
            };
            wl_resource_set_implementation(m_resource, &implementation, this, nullptr);
        }

        void failed()
        {
            orbital_surface_screenshot_send_failed(m_resource);
            wl_resource_destroy(m_resource);
            delete this;
        }

        void shoot(wl_resource *resource)
        {
            if (!wl_shm_buffer_get(resource)) {
                failed();
                return;
            }

            wl_shm_buffer *shm = wl_shm_buffer_get(resource);
            int width = wl_shm_buffer_get_width(shm);
            int height = wl_shm_buffer_get_height(shm);
            int stride = wl_shm_buffer_get_stride(shm);
            QSize size = m_surface->contentSize();

            if (stride != size.width() * 4 || height != size.height()) {
                failed();
                return;
            }

            wl_shm_buffer_begin_access(shm);

            void *data = wl_shm_buffer_get_data(shm);
            m_surface->copyContent(data, stride * height, QRect(0, 0, width, height));

            wl_shm_buffer_end_access(shm);

            orbital_surface_screenshot_send_done(m_resource);
            wl_resource_destroy(m_resource);
            delete this;
        }

        void motion(uint32_t time, Pointer::MotionEvent evt) override
        {
            pointer()->move(evt);
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            if (pointer()->buttonCount() == 0 && state == Pointer::ButtonState::Released) {
                View *view = m_compositor->pickView(pointer()->x(), pointer()->y());
                if (view) {
                    m_surface = view->surface();

                    QSize size = m_surface->contentSize();
                    orbital_surface_screenshot_send_setup(m_resource, size.width(), size.height(), size.width() * 4, 0);
                }

                end();
            }
        }

        Compositor *m_compositor;
        wl_resource *m_resource;
        Surface *m_surface;
    };

    auto *grab = new SurfaceScreenshot(m_compositor, res);
    Seat *seat = m_compositor->seats().front();
    seat->pointer()->setFocus(nullptr);
    grab->start(seat, PointerCursor::Kill);
}

}
