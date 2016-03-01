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
        , m_client(c->launchProcess(LIBEXEC_PATH "/orbital-authorizer-helper"))
        , m_resource(nullptr)
    {
        m_client->setAutoRestart(true);
    }
    ~Helper()
    {
        delete m_client;
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

        for (auto &req: m_pendingRequests) {
            sendAuthRequest(req.interface, req.pid, req.cb);
        }
    }

    void sendAuthRequest(const char *interface, pid_t pid, const std::function<void (int32_t)> &cb)
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

    void authRequested(const char *interface, pid_t pid, const std::function<void (int32_t)> &cb)
    {
        if (m_resource) {
            sendAuthRequest(interface, pid, cb);
        } else {
            m_pendingRequests.emplace_back(interface, pid, cb);
        }
    }

    Authorizer *m_auth;
    ChildProcess *m_client;
    wl_resource *m_resource;
    struct PendingRequest {
        PendingRequest(const char *iface, pid_t p, const std::function<void (int32_t)> &c) : interface(iface), pid(p), cb(c) {}
        const char *interface;
        pid_t pid;
        std::function<void (int32_t)> cb;
    };
    std::vector<PendingRequest> m_pendingRequests;
};


class TrustedClient
{
public:
    ~TrustedClient() { wl_list_remove(&listener.listener.link); }
    Authorizer *authorizer;
    wl_client *client;
    std::list<TrustedClient>::iterator iterator;
    std::list<TrustedClient> *list;
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
    delete m_helper;
}

void Authorizer::addRestrictedInterface(StringView interface)
{
    if (std::find(m_restrictedIfaces.begin(), m_restrictedIfaces.end(), interface) != std::end(m_restrictedIfaces)) {
        return;
    }
    m_restrictedIfaces.push_back(interface.toStdString());
}

void Authorizer::removeRestrictedInterface(StringView interface)
{
    for (auto i = m_restrictedIfaces.begin(); i != m_restrictedIfaces.end(); ++i) {
        if (*i == interface) {
            m_restrictedIfaces.erase(i);
            return;
        }
    }
}

void Authorizer::addTrustedClient(StringView interface, wl_client *c)
{
    auto &clients = m_trustedClients[interface.toStdString()];
    clients.emplace_front();
    TrustedClient &cl = clients.front();

    cl.iterator = clients.begin();
    cl.authorizer = this;
    cl.client = c;
    cl.list = &clients;
    cl.listener.parent = &cl;
    cl.listener.listener.notify = [](wl_listener *l, void *data)
    {
        TrustedClient *client = reinterpret_cast<TrustedClient::Listener *>(l)->parent;
        client->list->erase(client->iterator);
    };
    wl_client_add_destroy_listener(c, &cl.listener.listener);
}

bool Authorizer::isClientTrusted(StringView interface, wl_client *c) const
{
    auto it = m_trustedClients.find(interface.toStdString());
    if (it == m_trustedClients.end()) {
        return false;
    }

    for (const TrustedClient &cl: it->second) {
        if (cl.client == c) {
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

    if (std::find(m_restrictedIfaces.begin(), m_restrictedIfaces.end(), global) == std::end(m_restrictedIfaces)) {
        qDebug("Authorization request for unknown interface '%s'. Granting...", global);
        grant(resource);
        return;
    }

    pid_t pid;
    wl_client_get_credentials(client, &pid, nullptr, nullptr);

    qDebug("Authorization for global '%s' requested by process %d.", global, pid);

    std::string iface = global; //take a copy or 'global' may become invalid when the callback runs
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
