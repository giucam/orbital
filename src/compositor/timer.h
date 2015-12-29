/*
 * Copyright 2015 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_TIMER_H
#define ORBITAL_TIMER_H

#include <functional>

struct wl_event_source;

namespace Orbital {

class Timer
{
public:
    Timer();
    ~Timer();

    void setRepeat(bool repeat);
    void setTimeoutHandler(const std::function<void ()> &func);
    void start(int msecs, const std::function<void ()> &func);
    void start(int msecs);
    void stop();

    static void singleShot(int msecs, const std::function<void ()> &func);

private:
    void timeout();
    void rearm();

    std::function<void ()> m_func;
    wl_event_source *m_timerSource;
    wl_event_source *m_idleSource;
    int m_interval;
    bool m_repeat;
};

}

#endif
