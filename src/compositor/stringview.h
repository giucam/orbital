/*
 * Copyright 2015 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_STRINGVIEW_H
#define ORBITAL_STRINGVIEW_H

#include <string>

#include <QByteArray>
#include <QDebug>

namespace Orbital {

class StringView
{
public:
    StringView();
    StringView(const char *str);
    StringView(const char *str, size_t l);
    StringView(const std::string &str);
    StringView(const QByteArray &str);

    inline size_t size() const { return end - string; }

    std::string toStdString() const;

    bool operator==(StringView v) const;

private:
    const char *string;
    const char *end;

    friend QDebug operator<<(QDebug dbg, StringView v);
};

inline bool operator==(const std::string &str, StringView v) { return StringView(str) == v; }

inline QDebug operator<<(QDebug dbg, StringView v)
{
    QDebugStateSaver saver(dbg);
    dbg << QByteArray::fromRawData(v.string, v.size());
    return dbg;
}

};

#endif
