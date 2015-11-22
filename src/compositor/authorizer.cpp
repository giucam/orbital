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
#include "compositor.h"
#include "utils.h"
#include "wayland-authorizer-server-protocol.h"

namespace Orbital {

class TrustedClient
{
public:
    ~TrustedClient() { wl_list_remove(&listener.listener.link); }
    Authorizer *authorizer;
    wl_client *client;
    QByteArray interface;
    struct Listener {
        wl_listener listener;
        TrustedClient *parent;
    };
    Listener listener;
};


Authorizer::Authorizer(Compositor *compositor)
          : QObject(compositor)
          , Global(compositor, &orbital_authorizer_interface, 1)
{
}

Authorizer::~Authorizer()
{
    for (auto i = m_trustedClients.constBegin(); i != m_trustedClients.constEnd(); ++i) {
        qDeleteAll(i.value());
    }
}

void Authorizer::addRestrictedInterface(const QByteArray &interface)
{
    if (!m_restrictedIfaces.contains(interface)) {
        m_restrictedIfaces << interface;
    }
}

void Authorizer::removeRestrictedInterface(const QByteArray &interface)
{
    m_restrictedIfaces.removeOne(interface);
}

void Authorizer::addTrustedClient(const QByteArray &interface, wl_client *c)
{
    TrustedClient *cl = new TrustedClient;
    cl->authorizer = this;
    cl->client = c;
    cl->interface = interface;
    cl->listener.parent = cl;
    cl->listener.listener.notify = [](wl_listener *l, void *data)
    {
        TrustedClient *client = reinterpret_cast<TrustedClient::Listener *>(l)->parent;
        client->authorizer->m_trustedClients[client->interface].removeOne(client);
        delete client;
    };
    wl_client_add_destroy_listener(c, &cl->listener.listener);

    m_trustedClients[interface] << cl;
}

bool Authorizer::isClientTrusted(const QByteArray &interface, wl_client *c) const
{
    for (TrustedClient *cl: m_trustedClients.value(interface)) {
        if (cl->client == c) {
            return true;
        }
    }

    return false;
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

    if (!m_restrictedIfaces.contains(global)) {
        qDebug("Authorization request for unknown interface '%s'. Granting...", global);
        grant(resource);
        return;
    }

    pid_t pid;
    wl_client_get_credentials(client, &pid, nullptr, nullptr);

    char path[256], buf[256];
    int ret = snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    if ((size_t)ret >= sizeof(path)) {
        deny(resource);
        return;
    }

    ret = readlink(path, buf, sizeof(buf));
    if (ret == -1 || (size_t)ret == sizeof(buf)) {
        deny(resource);
        return;
    }
    buf[ret] = '\0';

    qDebug("Authorization for global '%s' requested by process '%s', pid %d.", global, buf, pid);

    bool granted = false;
    if (strcmp(global, "orbital_screenshooter") == 0) {
        granted = strcmp(buf, BIN_PATH "/orbital-screenshooter") == 0;
    }

    if (granted) {
        qDebug("Authorization granted.");
        grant(resource);
        addTrustedClient(global, client);
    } else {
        qDebug("Authorization denied.");
        deny(resource);
    }
}

void Authorizer::grant(wl_resource *res)
{
    orbital_authorizer_feedback_send_granted(res);
    wl_resource_destroy(res);
}

void Authorizer::deny(wl_resource *res)
{
    orbital_authorizer_feedback_send_denied(res);
    wl_resource_destroy(res);
}

}
