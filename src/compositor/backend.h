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

#ifndef ORBITAL_BACKEND_H
#define ORBITAL_BACKEND_H

#include <unordered_map>

#include <QObject>

#include "stringview.h"

class QPluginLoader;

struct weston_compositor;

namespace Orbital {

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend();

    virtual bool init(weston_compositor *c) = 0;
};

class BackendFactory
{
public:
    static void searchPlugins();
    static void cleanupPlugins();

    static Backend *createBackend(StringView name);

private:
    std::unordered_map<std::string, QPluginLoader *> m_factories;
};

}

Q_DECLARE_INTERFACE(Orbital::Backend, "Orbital.Compositor.Backend")

#endif
