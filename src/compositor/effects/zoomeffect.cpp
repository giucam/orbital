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

#include <QDebug>

#include "zoomeffect.h"
#include "../shell.h"
#include "../compositor.h"
#include "../global.h"
#include "../binding.h"
#include "../seat.h"
#include "../output.h"

namespace Orbital {

ZoomEffect::ZoomEffect(Shell *shell)
          : Effect(shell)
          , m_shell(shell)
{
    m_binding = shell->compositor()->createAxisBinding(PointerAxis::Vertical, KeyboardModifiers::Super);
    connect(m_binding, &AxisBinding::triggered, this, &ZoomEffect::run);
}

ZoomEffect::~ZoomEffect()
{
}

void ZoomEffect::run(Seat *seat, uint32_t time, PointerAxis axis, double value)
{
    for (Output *out: m_shell->compositor()->outputs()) {
        if (out->contains(seat->pointer()->x(), seat->pointer()->y())) {
            weston_output *output = out->output();
            /* For every pixel zoom 20th of a step */
            float increment = output->zoom.increment * -value / 20.f;

            output->zoom.level += increment;

            if (output->zoom.level < 0.f)
                output->zoom.level = 0.f;
            else if (output->zoom.level > output->zoom.max_level)
                output->zoom.level = output->zoom.max_level;
            else if (!output->zoom.active) {
                weston_output_activate_zoom(output);
            }

            output->zoom.spring_z.target = output->zoom.level;
            weston_output_update_zoom(output);
        }
    }
}

}
