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

#ifndef ORBITAL_ANIMATION_H
#define ORBITAL_ANIMATION_H

#include <memory>

#include <compositor.h>

#include "utils.h"

namespace Orbital {

class Output;

class BaseAnimation
{
public:
    explicit BaseAnimation();
    ~BaseAnimation();
    void destroy();

    void setSpeed(double speed);
    void run(Output *output, uint32_t duration);
    void run(Output *output);
    void stop();
    bool isRunning() const;
    template<class T>
    void setCurve(const T &curve) {
        m_curve = std::make_unique<Curve<T>>(curve);
    }

    Signal<> done;

protected:
    virtual void updateAnim(double value) = 0;

private:
    void tick(weston_output *output, uint32_t msecs);

    struct AnimationCurve
    {
        virtual ~AnimationCurve() {}
        virtual double value(double f) = 0;
    };
    template<class T>
    struct Curve : public AnimationCurve
    {
        Curve(const T &c) : curve(c) {}
        double value(double f) override { return curve.value(f); }
        T curve;
    };

    struct AnimWrapper {
        weston_animation ani;
        BaseAnimation *parent;
    };
    AnimWrapper m_animation;
    uint32_t m_duration;
    double m_speed;
    uint32_t m_timestamp;
    std::unique_ptr<AnimationCurve> m_curve;
};

template<class T>
class Animation : public BaseAnimation
{
public:
    Animation()
        : BaseAnimation()
    {
    }

    void setStart(const T &start) { m_start = start; }
    void setTarget(const T &target) { m_target = target; }

    Signal<T> update;

protected:
    void updateAnim(double v) override
    {
        T value = m_start * (1. - v) + m_target * v;
        update(value);
    }

private:
    T m_start;
    T m_target;
};

}

#endif
