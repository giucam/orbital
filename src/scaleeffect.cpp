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
#include "animation.h"

struct Grab : public ShellGrab {
    ScaleEffect *effect;
};

struct SurfaceTransform {
    void updateAnimation(float value);
    void doneAnimation();

    ShellSurface *surface;
    struct weston_transform transform;
    Animation *animation;

    float ss, ts;
    int sx, tx;
    int sy, ty;
};

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
    [](struct wl_pointer_grab *grab, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {},
    [](struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y) {},
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
    int num = m_surfaces.size();
    if (num == 0) {
        return;
    }

    const int ANIM_DURATION = 300;

    int numCols = ceil(sqrt(num));
    int numRows = ceil((float)num / (float)numCols);

    int r = 0, c = 0;
    for (SurfaceTransform *surf: m_surfaces) {
        if (!surf->surface->isMapped()) {
            continue;
        }

        if (m_scaled) {
            surf->ss = surf->ts;
            surf->ts = 1.f;

            surf->sx = surf->tx;
            surf->sy = surf->ty;
            surf->tx = surf->ty = 0.f;

            surf->animation->setStart(0.f);
            surf->animation->setTarget(1.f);
            surf->animation->run(surf->surface->output(), std::bind(&SurfaceTransform::updateAnimation, surf, std::placeholders::_1),
                                 std::bind(&SurfaceTransform::doneAnimation, surf), ANIM_DURATION);
        } else {
            int cellW = surf->surface->output()->width / numCols;
            int cellH = surf->surface->output()->height / numRows;

            float rx = (float)cellW / (float)surf->surface->width();
            float ry = (float)cellH / (float)surf->surface->height();
            if (rx > ry) {
                rx = ry;
            } else {
                ry = rx;
            }
            int x = c * cellW - surf->surface->x() + (cellW - (surf->surface->width() * rx)) / 2.f;
            int y = r * cellH - surf->surface->y() + (cellH - (surf->surface->height() * ry)) / 2.f;

            struct weston_matrix *matrix = &surf->transform.matrix;
            weston_matrix_init(matrix);
            weston_matrix_scale(matrix, surf->ss, surf->ss, 1.f);
            weston_matrix_translate(matrix, surf->sx, surf->sy, 0);

            surf->ts = rx;
            surf->tx = x;
            surf->ty = y;

            surf->animation->setStart(0.f);
            surf->animation->setTarget(1.f);
            surf->animation->run(surf->surface->output(), std::bind(&SurfaceTransform::updateAnimation, surf, std::placeholders::_1),
                                 ANIM_DURATION);

            surf->surface->addTransform(&surf->transform);
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
    SurfaceTransform *tr = new SurfaceTransform;
    tr->surface = surface;
    tr->animation = new Animation;

    wl_list_init(&tr->transform.link);

    tr->sx = tr->sy = 0;
    tr->ss = 1.f;

    m_surfaces.push_back(tr);

    if (m_scaled) {
        for (SurfaceTransform *surf: m_surfaces) {
            if (surf->surface != surface) {
                surf->ss = surf->ts;
                surf->sx = surf->tx;
                surf->sy = surf->ty;
            }
        }
        m_scaled = false;
        run(m_seat);
    }
}

void SurfaceTransform::updateAnimation(float value)
{
    struct weston_matrix *matrix = &transform.matrix;
    weston_matrix_init(matrix);
    float scale = ss + (ts - ss) * value;
    weston_matrix_scale(matrix, scale, scale, 1.f);

    weston_matrix_translate(matrix, sx + (float)(tx - sx) * value, sy + (float)(ty - sy) * value, 0);
    surface->damage();
}

void SurfaceTransform::doneAnimation()
{
    surface->removeTransform(&transform);
    sx = sy = 0;
    ss = 1.f;
}
