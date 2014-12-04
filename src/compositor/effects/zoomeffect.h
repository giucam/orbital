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

#ifndef ORBITAL_ZOOMEFFECT_H
#define ORBITAL_ZOOMEFFECT_H

#include "../effect.h"

namespace Orbital {

class Shell;
class Seat;
class AxisBinding;
enum class PointerAxis : unsigned char;

class ZoomEffect : public Effect
{
public:
    ZoomEffect(Shell *shell);
    ~ZoomEffect();

private:
    void run(Seat *seat, uint32_t time, PointerAxis axis, double value);

    Shell *m_shell;
    AxisBinding *m_binding;
};

}

#endif
