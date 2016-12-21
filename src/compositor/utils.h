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

#include <utility>
#include <functional>
#include <vector>

#include <wayland-server-core.h>

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

    inline bool isSet() const { return m_isSet; }
    inline operator bool() const { return m_isSet; }

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



class Listener : wl_listener
{
public:
    using Notify = std::function<void (Listener *, void *)>;
    inline Listener()
    {
        wl_list_init(&link);
        notify = fire;
    }
    inline ~Listener() { wl_list_remove(&link); }

    inline void setNotify(const Notify &n) { m_notify = n; }

    inline void connect(wl_signal *signal) { wl_signal_add(signal, this); }

private:
    inline static void fire(wl_listener *listener, void *data) {
        auto *l = static_cast<Listener *>(listener);
        l->m_notify(l, data);
    };

    Notify m_notify;
};


template<class... Args>
class Signal
{
public:
    void connect(const std::function<void (Args...)> &f)
    {
        m_slots.push_back(f);
    }
    template<class T, class F, class... FArgs>
    void connect(T *obj, void (F::*func)(FArgs...)) {
        static_assert(std::is_base_of<F, T>::value);
        m_slots.push_back([obj, func](const Args &... args) {
            (obj->*func)(args...);
        });
    }

    void operator()(const Args &... args) const
    {
        for (auto &&s: m_slots) {
            s(args...);
        }
    }

private:
    std::vector<std::function<void (Args...)>> m_slots;
};

template<class... Args>
struct Overload
{
    template<class R, class T>
    constexpr inline auto operator()(R (T::*f)(Args...)) const -> decltype(f) { return f; }
};

template<class... Args>
constexpr Overload<Args...> overload = {};

#endif
