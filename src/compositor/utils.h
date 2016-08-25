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

#ifndef ORBITAL_UTILS_H
#define ORBITAL_UTILS_H

#include <compositor.h>

namespace Orbital {

template<class F>
struct InterfaceWrapper {};

template<class R, class T, class... Args>
struct InterfaceWrapper<R (T::*)(Args...)> {
    template<R (T::*F)(Args...)>
    static constexpr void forward(wl_client *, wl_resource *resource, Args... args) {
        (static_cast<T *>(wl_resource_get_user_data(resource))->*F)(std::forward<Args>(args)...);
    }
};

template<class R, class T, class... Args>
struct InterfaceWrapper<R (T::*)(wl_client *, wl_resource *, Args...)> {
    template<R (T::*F)(wl_client *, wl_resource *, Args...)>
    static constexpr void forward(wl_client *client, wl_resource *resource, Args... args) {
        (static_cast<T *>(wl_resource_get_user_data(resource))->*F)(client, resource, std::forward<Args>(args)...);
    }
};

#define wrapExtInterface(method) InterfaceWrapper<decltype(method)>::forward<method>
#define wrapInterface(method) \
InterfaceWrapper<decltype(&std::remove_pointer_t<decltype(this)>::method)>::forward<&std::remove_pointer_t<decltype(this)>::method>

template<class T>
class Maybe
{
public:
    inline Maybe() : m_isSet(false) {}
    inline Maybe(T v) : m_value(v), m_isSet(true) {}
    inline Maybe(const Maybe<T> &m) : m_value(m.m_value), m_isSet(m.m_isSet) {}

    inline bool isSet() const { return m_isSet; }
    inline operator bool() const { return m_isSet; }
    inline Maybe<T> &operator=(const Maybe<T> &m) { m_value = m.m_value; m_isSet = m.m_isSet; return *this; }

    inline void set(const T &v) { m_value = v; m_isSet = true; }
    inline void reset() { m_isSet = false; }
    inline const T &value() const { return m_value; }

private:
    T m_value;
    bool m_isSet;
};

}


#define DECLARE_OPERATORS_FOR_FLAGS(F) \
    inline int operator&(F a, F b) { \
        return (int)a & (int)b; \
    } \
    inline F operator|(F a, F b) { return (F)((int)a | (int)b); }

#endif
