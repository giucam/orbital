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

#include "stringview.h"

namespace Orbital {

StringView::StringView()
          : string(nullptr)
          , end(nullptr)
{
}

StringView::StringView(const char *str, size_t l)
          : string(str)
          , end(str + l)
{
}

StringView::StringView(const char *str)
          : StringView(str, str ? strlen(str) : 0)
{
}

StringView::StringView(const std::string &str)
          : StringView(str.data(), str.size())
{
}

StringView::StringView(const QByteArray &str)
          : StringView(str.constData(), str.size())
{
}

std::string StringView::toStdString() const
{
    return std::string(string, size());
}

QString StringView::toQString() const
{
    return QString::fromUtf8(string, size());
}

void StringView::split(char c, const std::function<bool (StringView substr)> &func) const
{
    if (!string || string == end) {
        return;
    }

    const char *substr = string;
    const char *p = string;
    do {
        if (*p == c || p == end) {
            if (p == end && substr == string) { //there was no 'c' in the string, bail out
                break;
            }
            int l = p - substr;
            if (l && func(StringView(substr, l))) {
                break;
            }
            substr = p + 1;
        }
        p++;
    } while (substr <= end);
}

bool StringView::operator==(StringView v) const
{
    return size() == v.size() && memcmp(string, v.string, size()) == 0;
}

}
