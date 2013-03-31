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

#include "animation.h"

Animation::Animation()
{
    wl_list_init(&m_animation.link);
    m_animation.frame = [](struct weston_animation *base, struct weston_output *output, uint32_t msecs) {
        Animation *animation = container_of(base, Animation, m_animation);
        animation->update(output, msecs);
    };
}

void Animation::setStart(float value)
{
    m_start = value;
}

void Animation::setTarget(float value)
{
    m_target = value;
}

void Animation::run(struct weston_output *output, const std::function<void (float)> &handler, uint32_t duration)
{
    run(output, handler, nullptr, duration);
}

void Animation::run(struct weston_output *output, const std::function<void (float)> &handler,
                    const std::function<void ()> &doneHandler, uint32_t duration)
{
    m_handler = handler;
    m_doneHandler = doneHandler;
    m_duration = duration;

    m_animation.frame_counter = 0;

    wl_list_insert(&output->animation_list, &m_animation.link);
    weston_compositor_schedule_repaint(output->compositor);
}

void Animation::update(struct weston_output *output, uint32_t msecs)
{
    if (m_animation.frame_counter <= 1) {
        m_timestamp = msecs;
    }

    uint32_t time = msecs - m_timestamp;
    if (time > m_duration) {
        m_handler(m_target);
        if (m_doneHandler) {
            m_doneHandler();
        }
        wl_list_remove(&m_animation.link);
        wl_list_init(&m_animation.link);
        weston_compositor_schedule_repaint(output->compositor);
        return;
    }

    float f = (float)time / (float)m_duration;
    m_handler(m_target * f + m_start * (1.f - f));

    weston_compositor_schedule_repaint(output->compositor);
}