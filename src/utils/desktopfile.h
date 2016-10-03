/*
 * Copyright 2016 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_DESKTOPFILE_H
#define ORBITAL_DESKTOPFILE_H

#include <string>
#include <unordered_map>

#include "stringview.h"

namespace Orbital {

class DesktopFile;

class DesktopFile
{
public:
    DesktopFile(StringView file);

    inline bool isValid() const { return m_valid; }

    void beginGroup(StringView name);
    void endGroup();

    bool hasValue(StringView key) const;
    StringView value(StringView key, StringView defaultValue = "") const;

    template<class T>
    T value(StringView key) const { T t; desktopEntryValue(*this, key, t); return t; }

private:
    bool m_valid;
    std::unordered_map<std::string, std::string> m_data;
    std::string m_group;
};

inline void desktopEntryValue(const DesktopFile &d, StringView key, bool &value)
{
    value = d.hasValue(key) && d.value(key) == "true";
}

}

#endif
