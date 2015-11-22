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

#ifndef ORBITAL_AUTHORIZER_H
#define ORBITAL_AUTHORIZER_H

#include "interface.h"

struct wl_resource;

namespace Orbital {

class Shell;

class Authorizer : public Interface, public Global
{
public:
    explicit Authorizer(Shell *shell);

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    void destroy(wl_client *client, wl_resource *res);
    void authorize(wl_client *client, wl_resource *res, uint32_t id, const char *global);
};

}

#endif
