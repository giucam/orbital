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

Animation::Animation(QObject *p)
         : QObject(p)
         , m_speed(-1.)
         , m_curve(nullptr)
{
    m_animation.parent = this;
    wl_list_init(&m_animation.ani.link);
    m_animation.ani.frame = [](weston_animation *base, weston_output *output, uint32_t msecs) {
        AnimWrapper *animation = container_of(base, AnimWrapper, ani);
        animation->parent->tick(output, msecs);
    };
}

Animation::~Animation()
{
    stop();
    delCurve();
}

void Animation::setStart(double value)
{
    m_start = value;
}

void Animation::setTarget(double value)
{
    m_target = value;
}

void Animation::setSpeed(double speed)
{
    m_speed = speed;
}

void Animation::run(Output *output, uint32_t duration, Animation::Flags flags)
{
    stop();

    if (!output) {
        emit update(m_target);
        if (flags & Flags::SendDone) {
            emit done();
        }
        return;
    }

    m_duration = duration;
    m_runFlags = flags;
    m_animation.ani.frame_counter = 0;

    wl_list_insert(&output->m_output->animation_list, &m_animation.ani.link);
    weston_output_schedule_repaint(output->m_output);

    emit update(m_start);
}

void Animation::run(Output *output, Animation::Flags flags)
{
    uint32_t duration;
    if (m_speed < 0.) {
        duration = 250;
    } else {
        duration = qAbs((m_target - m_start) / m_speed);
    }
    run(output, duration, flags);
}

void Animation::stop()
{
    if (isRunning()) {
        wl_list_remove(&m_animation.ani.link);
        wl_list_init(&m_animation.ani.link);
    }
}

bool Animation::isRunning() const
{
    return !wl_list_empty(&m_animation.ani.link);
}

void Animation::tick(weston_output *output, uint32_t msecs)
{
    if (m_animation.ani.frame_counter <= 1) {
        m_timestamp = msecs;
    }

    uint32_t time = msecs - m_timestamp;
    if (time > m_duration) {
        emit update(m_target);
        stop();
        weston_compositor_schedule_repaint(output->compositor);
        if (Flags::SendDone & m_runFlags) {
            emit done();
        }
        return;
    }

    double f = (double)time / (double)m_duration;
    if (m_curve) {
        f = m_curve->value(f);
    }
    emit update(m_target * f + m_start * (1.f - f));

    weston_compositor_schedule_repaint(output->compositor);
}

void Animation::delCurve()
{
    delete m_curve;
}

}
