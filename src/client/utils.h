/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef UTILS_H
#define UTILS_H

class QWindow;

struct wl_surface;

wl_surface *nativeSurface(QWindow *window);

template<class T, class... Args>
struct Wrapper {
    template<void (T::*F)(Args...)>
    static void forward(void *data, Args... args) {
        (static_cast<T *>(data)->*F)(args...);
    }
};

template<class T, class... Args>
constexpr static auto createWrapper(void (T::*func)(Args...)) -> Wrapper<T, Args...> {
    return Wrapper<T, Args...>();
}

#define wrapInterface(method) createWrapper(method).forward<method>

#endif
