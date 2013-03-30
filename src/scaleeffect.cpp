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

#include "scaleeffect.h"
#include "shellsurface.h"
#include "shell.h"

struct Grab : public ShellGrab {
    ScaleEffect *effect;
};

static void grab_focus(struct wl_pointer_grab *grab, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
//     grab->focus = surface;
}

void grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{

}

void grab_button(struct wl_pointer_grab *base, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shgrab = container_of(base, ShellGrab, grab);
    Grab *grab = static_cast<Grab *>(shgrab);
    if (state_w == WL_POINTER_BUTTON_STATE_PRESSED) {
        struct weston_surface *surface = (struct weston_surface *) base->pointer->current;
        ShellSurface *shsurf = grab->shell->getShellSurface(surface);
        if (shsurf) {
            grab->effect->end(shsurf);
        }
    }
}

static const struct wl_pointer_grab_interface grab_interface = {
    grab_focus,
    grab_motion,
    grab_button,
};

ScaleEffect::ScaleEffect(Shell *shell)
           : Effect(shell)
           , m_scaled(false)
           , m_grab(new Grab)
{
    m_grab->effect = this;
}

void ScaleEffect::run(struct wl_seat *seat, uint32_t time, uint32_t key)
{
    run((struct weston_seat *)seat);
}

void ScaleEffect::run(struct weston_seat *ws)
{
    int num = surfaces().size();
    if (num == 0) {
        return;
    }

    int sq = sqrt(num);
    int numRows = sq;
    int numCols = num / numRows;
    if (numRows * numCols < num)
        ++numCols;
    int r = 0, c = 0;
    for (SurfaceTransform &surf: surfaces()) {
        if (!surf.surface->isMapped()) {
            continue;
        }

        if (m_scaled) {
            surf.surface->removeTransform(&surf.transform);
        } else {
            int cellW = surf.surface->output()->width / numCols;
            int cellH = surf.surface->output()->height / numRows;

            float rx = (float)cellW / (float)surf.surface->width();
            float ry = (float)cellH / (float)surf.surface->height();
            if (rx > ry) {
                rx = ry;
            } else {
                ry = rx;
            }
            int x = c * cellW - surf.surface->x() + (cellW - (surf.surface->width() * rx)) / 2.f;
            int y = r * cellH - surf.surface->y() + (cellH - (surf.surface->height() * ry)) / 2.f;

            struct weston_matrix *matrix = &surf.transform.matrix;
            weston_matrix_init(matrix);
            weston_matrix_translate(matrix, x / rx, y / ry, 0);
            weston_matrix_scale(matrix, rx, ry, 1.f);
            surf.surface->addTransform(&surf.transform);
        }
        if (++c >= numCols) {
            c = 0;
            ++r;
        }
    }
    m_scaled = !m_scaled;
    if (m_scaled) {
        m_seat = ws;
        shell()->startGrab(m_grab, &grab_interface, ws->seat.pointer/*, DESKTOP_SHELL_CURSOR_MOVE*/);
    } else {
        m_seat = nullptr;
        Shell::endGrab(m_grab);
    }
}

void ScaleEffect::end(ShellSurface *surface)
{
    shell()->activateSurface(surface, m_seat);
    run(m_seat);
}

void ScaleEffect::addedSurface(ShellSurface *surface)
{
    if (m_scaled) {
        for (SurfaceTransform &surf: surfaces()) {
            if (surf.surface != surface) {
                surf.surface->removeTransform(&surf.transform);
            }
        }
        m_scaled = false;
        run(m_seat);
    }
}
