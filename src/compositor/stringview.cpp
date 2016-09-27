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

bool StringView::contains(int c) const
{
    return memchr(string, c, end - string);
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
    do {
        const char *p = (const char *)memchr(substr, c, end - substr);
        if (p) {
            // if *p is a multibyte char memchr returns a pointer to the last byte of the character.
            // so here we go back until we find the first one
            int wide = (*p >> 7) & 0x1;
            while (wide && p > substr) {
                bool isStart = (*--p >> 7) & 0x1 && (*p >> 6) & 0x1;
                if (isStart) {
                    break;
                }
            }
        } else {
            // there is no 'c' in the string, so run through to the end
            p = end;
        }

        size_t l = p - substr;
        if (l && func(StringView(substr, l))) {
            break;
        }

        substr = p + 1;
        int wide = (*p >> 7) & 0x1;

        int i = 0;
        if (wide) {
            while (wide && i < 4) {
                //we need to advance to the next character, so we need to account for multibyte chars.
                //count the number of bytes of this char and advance by that
                wide = (*p >> (7 - ++i)) & 0x1;
            }
            substr += i-1;
        }
    } while (substr <= end);
}

bool StringView::operator==(StringView v) const
{
    return size() == v.size() && memcmp(string, v.string, size()) == 0;
}

}
