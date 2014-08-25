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

#include <QObject>

#include <weston/compositor.h>

#include "utils.h"

namespace Orbital {

class AnimationCurve;
class Output;

class Animation : public QObject
{
    Q_OBJECT
public:
    enum class Flags {
        None = 0,
        SendDone = 1
    };
    explicit Animation(QObject *p = nullptr);
    ~Animation();
    void destroy();

    void setStart(double value);
    void setTarget(double value);
    void run(Output *output, uint32_t duration, Flags flags = Flags::None);
    void stop();
    bool isRunning() const;
    template<class T>
    void setCurve(const T &curve) { delCurve(); m_curve = new T; *static_cast<T *>(m_curve) = curve; }

signals:
    void update(double value);
    void done();

private:
    void tick(weston_output *output, uint32_t msecs);
    void delCurve();

    struct AnimWrapper {
        weston_animation ani;
        Animation *parent;
    };
    AnimWrapper m_animation;
    double m_start;
    double m_target;
    uint32_t m_duration;
    uint32_t m_timestamp;
    Flags m_runFlags;
    AnimationCurve *m_curve;
};

}

DECLARE_OPERATORS_FOR_FLAGS(Orbital::Animation::Flags)

#endif
