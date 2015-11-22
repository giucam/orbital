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

#include <unistd.h>

#include "authorizer.h"
#include "shell.h"
#include "utils.h"
#include "wayland-authorizer-server-protocol.h"

namespace Orbital {

Authorizer::Authorizer(Shell *shell)
          : Interface(shell)
          , Global(shell->compositor(), &orbital_authorizer_interface, 1)
{
}

void Authorizer::bind(wl_client *client, uint32_t version, uint32_t id)
{
    static const struct orbital_authorizer_interface implementation = {
        wrapInterface(&Authorizer::destroy),
        wrapInterface(&Authorizer::authorize)
    };

    wl_resource *resource = wl_resource_create(client, &orbital_authorizer_interface, version, id);
    wl_resource_set_implementation(resource, &implementation, this, nullptr);
}

void Authorizer::destroy(wl_client *client, wl_resource *res)
{
    wl_resource_destroy(res);
}

void Authorizer::authorize(wl_client *client, wl_resource *res, uint32_t id, const char *global)
{
    wl_resource *resource = wl_resource_create(client, &orbital_authorizer_feedback_interface, wl_resource_get_version(res), id);

    pid_t pid;
    wl_client_get_credentials(client, &pid, nullptr, nullptr);

    char path[256], buf[256];
    int ret = snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    if ((size_t)ret >= sizeof(path)) {
        orbital_authorizer_feedback_send_denied(resource);
        wl_resource_destroy(resource);
        return;
    }

    ret = readlink(path, buf, sizeof(buf));
    if (ret == -1 || (size_t)ret == sizeof(buf)) {
        orbital_authorizer_feedback_send_denied(resource);
        wl_resource_destroy(resource);
        return;
    }
    buf[ret] = '\0';

    qDebug("Authorization for global '%s' requested by process '%s', pid %d.", global, buf, pid);

    bool granted = false;
    if (strcmp(global, "orbital_screenshooter") == 0) {
        granted = strcmp(buf, LIBEXEC_PATH "/orbital-screenshooter") == 0;
    }

    if (granted) {
        qDebug("Authorization granted.");
        orbital_authorizer_feedback_send_granted(resource);
        static_cast<Shell *>(object())->addTrustedClient(global, client);
    } else {
        orbital_authorizer_feedback_send_denied(resource);
        wl_resource_destroy(resource);
    }
}

}
