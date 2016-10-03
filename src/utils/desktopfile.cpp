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

#include <fstream>
#include <sstream>

#include "desktopfile.h"

namespace Orbital {

DesktopFile::DesktopFile(StringView file)
           : m_valid(false)
{
    std::ifstream stream(file.toStdString(), std::ifstream::in);
    if (!stream.good()) {
        return;
    }

    std::string currentGroup;

    while (stream.good()) {
        std::stringbuf lineBuf;
        stream.get(lineBuf);
        stream.ignore(1); //discard the'\n'
        std::string line = lineBuf.str();

        int length = line.size();
        if (line.find_first_of('\n') != std::string::npos) {
            --length;
        }
        if (length == 0) {
            continue;
        }

        if (line.find_first_of('[') == 0) {
            int idx = line.find_first_of(']');
            if (idx == -1 || idx != length - 1) {
                return;
            }
            currentGroup = line.substr(1, length - 2);
            continue;
        }

        int idx = line.find_first_of('=');
        if (idx < 1) {
            return;
        }
        std::string key = line.substr(0, idx);
        std::string value = line.substr(idx + 1, length - idx - 1);

        m_data[currentGroup + '/' + key] = value;
    }

    m_valid = true;
}

void DesktopFile::beginGroup(StringView name)
{
    m_group = name.toStdString();
}

void DesktopFile::endGroup()
{
    m_group.clear();
}

bool DesktopFile::hasValue(StringView key) const
{
    if (m_group.empty()) {
        return m_data.count(key.toStdString());
    }

    return m_data.count(m_group + "/" + key.toStdString());
}

StringView DesktopFile::value(StringView key, StringView defaultValue) const
{
    if (m_group.empty()) {
        return m_data.at(key.toStdString());
    }
    auto it = m_data.find(m_group + "/" + key.toStdString());
    if (it == m_data.end()) {
        return defaultValue;
    }
    return it->second;
}

}
