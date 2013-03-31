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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <functional>

#include <weston/compositor.h>

class ShellSurface;

class Animation {
public:
    explicit Animation();

    void setStart(float value);
    void setTarget(float value);
    void run(struct weston_output *output, const std::function<void (float)> &handler, uint32_t duration);
    void run(struct weston_output *output, const std::function<void (float)> &handler,
             const std::function<void ()> &done_handler, uint32_t duration);

private:
    void update(struct weston_output *output, uint32_t msecs);

    struct weston_animation m_animation;
    float m_start;
    float m_target;
    uint32_t m_duration;
    uint32_t m_timestamp;
    std::function<void (float)> m_handler;
    std::function<void ()> m_doneHandler;
};

#endif
