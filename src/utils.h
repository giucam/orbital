/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef UTILS_H
#define UTILS_H

template<typename T>
class Vector2D {
public:
    inline Vector2D(T a, T b) : x(a), y(b) {}
    T x;
    T y;
};

template<typename T>
class Rect2D {
public:
    inline Rect2D(T a, T b, T w, T h) : x(a), y(b), width(w), height(h) {}
    T x;
    T y;
    T width;
    T height;
};

typedef Vector2D<int> IVector2D;
typedef Rect2D<int> IRect2D;

#endif
