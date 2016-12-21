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

#include "animation.h"
#include "animationcurve.h"
#include "output.h"

namespace Orbital {

BaseAnimation::BaseAnimation()
         : m_speed(-1.)
         , m_curve(nullptr)
{
    m_animation.parent = this;
    wl_list_init(&m_animation.ani.link);
    m_animation.ani.frame = [](weston_animation *base, weston_output *output, uint32_t msecs) {
        AnimWrapper *animation = wl_container_of(base, (AnimWrapper *)nullptr, ani);
        animation->parent->tick(output, msecs);
    };
}

BaseAnimation::~BaseAnimation()
{
    stop();
}

void BaseAnimation::setSpeed(double speed)
{
    m_speed = speed;
}

void BaseAnimation::run(Output *output, uint32_t duration)
{
    stop();

    if (!output || duration == 0) {
        updateAnim(1.);
        done();
        return;
    }

    m_duration = duration;
    m_animation.ani.frame_counter = 0;

    wl_list_insert(&output->m_output->animation_list, &m_animation.ani.link);
    weston_output_schedule_repaint(output->m_output);

    updateAnim(0.);
}

void BaseAnimation::run(Output *output)
{
    uint32_t duration;
    if (m_speed < 0.) {
        duration = 250;
    } else {
        duration = qAbs(1. / m_speed);
    }
    run(output, duration);
}

void BaseAnimation::stop()
{
    if (isRunning()) {
        wl_list_remove(&m_animation.ani.link);
        wl_list_init(&m_animation.ani.link);
    }
}

bool BaseAnimation::isRunning() const
{
    return !wl_list_empty(&m_animation.ani.link);
}

void BaseAnimation::tick(weston_output *output, uint32_t msecs)
{
    if (m_animation.ani.frame_counter <= 1) {
        m_timestamp = msecs;
    }

    uint32_t time = msecs - m_timestamp;
    if (time > m_duration) {
        updateAnim(1.);
        stop();
        weston_compositor_schedule_repaint(output->compositor);
        done();
        return;
    }

    double f = (double)time / (double)m_duration;
    if (m_curve) {
        f = m_curve->value(f);
    }
    updateAnim(f);

    weston_compositor_schedule_repaint(output->compositor);
}

}
