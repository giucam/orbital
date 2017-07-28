/*
 * Copyright 2017 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef DEBUG_H
#define DEBUG_H

#include "fmt/format.h"
#include "fmt/ostream.h"

#include <QRect>

namespace Orbital {

class Debug
{
public:
    template<class... Args>
    static void debug(Args &&... args) {
        if (s_debugActive) {
            fmt::print(stderr, std::forward<Args>(args)...);
            fmt::print(stderr, "\n");
        }
    }

    static void toggleDebugOutput();

private:
    static bool s_debugActive;
};

}

inline std::ostream &operator<<(std::ostream &os, const QRect &rect) {
    os << "(" << rect.x() << "," << rect.y() << " " << rect.width() << "x" << rect.height() << ")";
    return os;
}

#endif
