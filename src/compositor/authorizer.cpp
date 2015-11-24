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

#include <functional>

#include <QDebug>

#include "authorizer.h"
#include "compositor.h"
#include "utils.h"
#include "wayland-authorizer-server-protocol.h"
#include "wayland-authorizer-helper-server-protocol.h"

namespace Orbital {


class Helper: public Global
{
public:
    Helper(Compositor *c, Authorizer *auth)
        : Global(c, &orbital_authorizer_helper_interface, 1)
        , m_auth(auth)
        , m_client(c->launchProcess(QStringLiteral(LIBEXEC_PATH "/orbital-authorizer-helper")))
        , m_resource(nullptr)
    {
        m_client->setAutoRestart(true);
    }

    void bind(wl_client *client, uint32_t version, uint32_t id) override
    {
        wl_resource *res = wl_resource_create(client, &orbital_authorizer_helper_interface, version, id);
        if (client != m_client->client()) {
            wl_resource_post_error(res, WL_DISPLAY_ERROR_INVALID_OBJECT, "permission to bind orbital_authorizer_feedback_interface denied");
            wl_resource_destroy(res);
            return;
        }
        m_resource = res;
    }

    void authRequested(const char *interface, pid_t pid, const std::function<void (int32_t)> &cb)
    {
        class Request {
        public:
            Request(wl_resource *r, const std::function<void (int32_t)> &cb)
                : res(r)
                , callback(cb)
            {
                static const struct orbital_authorizer_helper_result_interface impl = {
                    wrapInterface(&Request::result)
                };
                wl_resource_set_implementation(res, &impl, this, nullptr);
            }
            void result(int32_t result)
            {
                callback(result);
                wl_resource_destroy(res);
                delete this;
            }

            wl_resource *res;
            std::function<void (int32_t)> callback;
        };

        wl_resource *res = wl_resource_create(m_client->client(), &orbital_authorizer_helper_result_interface,
                                            wl_resource_get_version(m_resource), 0);
        orbital_authorizer_helper_send_authorization_requested(m_resource, res, interface, pid);
        new Request(res, cb);
    }

    Authorizer *m_auth;
    ChildProcess *m_client;
    wl_resource *m_resource;
};


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
          , m_helper(new Helper(compositor, this))
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

    qDebug("Authorization for global '%s' requested by process %d.", global, pid);

    QByteArray iface = global; //take a copy or 'global' may become invalid when the callback runs
    m_helper->authRequested(global, pid, [this, iface, client, resource](int32_t result) {
        if (result == 1) {
            qDebug("Authorization granted.");
            grant(resource);
            addTrustedClient(iface, client);
        } else {
            qDebug("Authorization denied.");
            deny(resource);
        }
    });
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
