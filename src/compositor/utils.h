/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ORBITAL_UTILS_H
#define ORBITAL_UTILS_H

#include <weston/compositor.h>

namespace Orbital {

template<class R, class T, class... Args>
struct Wrapper {
    template<R (T::*F)(wl_client *, wl_resource *, Args...)>
    static void forward(wl_client *client, wl_resource *resource, Args... args) {
        (static_cast<T *>(wl_resource_get_user_data(resource))->*F)(client, resource, args...);
    }
    template<R (T::*F)(Args...)>
    static void forward(wl_client *client, wl_resource *resource, Args... args) {
        (static_cast<T *>(wl_resource_get_user_data(resource))->*F)(args...);
    }
};

template<class R, class T, class... Args>
constexpr static auto createWrapper(R (T::*func)(wl_client *client, wl_resource *resource, Args...)) -> Wrapper<R, T, Args...> {
    return Wrapper<R, T, Args...>();
}

template<class R, class T, class... Args>
constexpr static auto createWrapper(R (T::*func)(Args...)) -> Wrapper<R, T, Args...> {
    return Wrapper<R, T, Args...>();
}

}

#define wrapInterface(method) createWrapper(method).forward<method>


#define DECLARE_OPERATORS_FOR_FLAGS(F) \
    inline int operator&(F a, F b) { \
        return (int)a & (int)b; \
    }

#endif
