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

#ifndef ORBITAL_GLOBAL_H
#define ORBITAL_GLOBAL_H

#include "utils.h"
#include "stringview.h"

namespace Orbital {

enum class KeyboardModifiers : unsigned char {
    None = 0,
    Ctrl = (1 << 0),
    Alt = (1 << 1),
    Super = (1 << 2),
    Shift = (1 << 3),
};
DECLARE_OPERATORS_FOR_FLAGS(KeyboardModifiers)

enum class PointerButton : unsigned char {
    Left = 0,
    Right = 1,
    Middle = 2,
    Side = 3,
    Extra = 4,
    Forward = 5,
    Back = 6,
    Task = 7,
    Extra2 = 8,
};

enum class PointerAxis : unsigned char {
    Vertical = 0,
    Horizontal = 1
};

uint32_t pointerButtonToRaw(PointerButton b);
PointerButton rawToPointerButton(uint32_t b);

class Keymap
{
public:
    Keymap() {}
    Keymap(const Maybe<StringView> &layout, const Maybe<StringView> &options);

    const Maybe<std::string> &layout() const { return m_layout; }
    const Maybe<std::string> &options() const { return m_options; }

    void fill(const Keymap &other);

private:
    Maybe<std::string> m_layout;
    Maybe<std::string> m_options;
};


}

#endif
