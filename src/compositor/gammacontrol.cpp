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

#include <QDebug>

#include "gammacontrol.h"
#include "shell.h"
#include "utils.h"
#include "output.h"
#include "wayland-gammacontrol-server-protocol.h"

namespace Orbital {

GammaControlManager::GammaControlManager(Shell *shell)
                   : Interface(shell)
                   , RestrictedGlobal(shell->compositor(), &gamma_control_manager_interface, 1)
{

}

GammaControlManager::~GammaControlManager()
{
}

void GammaControlManager::bind(wl_client *client, uint32_t version, uint32_t id)
{
    static const struct gamma_control_manager_interface implementation = {
        wrapInterface(destroy),
        wrapInterface(getGammaControl)
    };

    wl_resource *resource = wl_resource_create(client, &gamma_control_manager_interface, version, id);
    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void GammaControlManager::destroy(wl_client *client, wl_resource *res)
{
    wl_resource_destroy(res);
}

void GammaControlManager::getGammaControl(wl_client *client, wl_resource *res, uint32_t id, wl_resource *outputRes)
{
    Output *output = Output::fromResource(outputRes);

    class GammaControl
    {
    public:
        GammaControl(Output *o)
            : output(o)
        {
        }
        void destroy(wl_client *c, wl_resource *r)
        {
            wl_resource_destroy(r);
        }
        void setGamma(wl_client *c, wl_resource *res, wl_array *red, wl_array *green, wl_array *blue)
        {
            if (red->size != green->size || red->size != blue->size) {
                wl_resource_post_error(res, GAMMA_CONTROL_ERROR_INVALID_GAMMA, "The gamma ramps don't have the same size");
                return;
            }

            uint16_t *r = (uint16_t *)red->data;
            uint16_t *g = (uint16_t *)green->data;
            uint16_t *b = (uint16_t *)blue->data;
            output->setGamma(red->size / sizeof(uint16_t), r, g, b);
        }
        void resetGamma(wl_client *c, wl_resource *res)
        {

        }

        Output *output;
    };

    static const struct gamma_control_interface implementation = {
        wrapExtInterface(&GammaControl::destroy),
        wrapExtInterface(&GammaControl::setGamma),
        wrapExtInterface(&GammaControl::resetGamma)
    };
    GammaControl *gc = new GammaControl(output);

    wl_resource *resource = wl_resource_create(client, &gamma_control_interface, wl_resource_get_version(res), id);
    wl_resource_set_implementation(resource, &implementation, gc, [](wl_resource *r) {
        delete static_cast<GammaControl *>(wl_resource_get_user_data(r));
    });

    gamma_control_send_gamma_size(resource, output->gammaSize());
}

}
