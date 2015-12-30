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

#include "clipboard.h"
#include "shell.h"
#include "seat.h"
#include "compositor.h"
#include "utils.h"

#include "wayland-clipboard-server-protocol.h"

namespace Orbital {

ClipboardManager::ClipboardManager(Shell *shell)
                : Interface(shell)
                , Global(shell->compositor(), &orbital_clipboard_manager_interface, 1)
{
    Compositor *c = shell->compositor();
    for (Seat *s: c->seats()) {
        connect(s, &Seat::selection, this, &ClipboardManager::selection);
    }
    connect(c, &Compositor::seatCreated, [this](Seat *s) {
        connect(s, &Seat::selection, this, &ClipboardManager::selection);
    });
}

ClipboardManager::~ClipboardManager()
{
}

void ClipboardManager::bind(wl_client *client, uint32_t version, uint32_t id)
{
    static const struct orbital_clipboard_manager_interface implementation = {
        wrapInterface(&ClipboardManager::destroy)
    };

    wl_resource *resource = wl_resource_create(client, &orbital_clipboard_manager_interface, version, id);
    wl_resource_set_implementation(resource, &implementation, this, [](wl_resource *r) {
        auto *_this = static_cast<ClipboardManager *>(wl_resource_get_user_data(r));
        auto it = std::find(_this->m_resources.begin(), _this->m_resources.end(), r);
        if (it != _this->m_resources.end()) {
            _this->m_resources.erase(it);
        }
    });
    m_resources.push_back(resource);
}

void ClipboardManager::destroy(wl_client *client, wl_resource *res)
{
    wl_resource_destroy(res);
}

void ClipboardManager::selection(Seat *seat)
{
    for (wl_resource *r: m_resources) {
        seat->sendSelection(wl_resource_get_client(r));
    }
}

}
