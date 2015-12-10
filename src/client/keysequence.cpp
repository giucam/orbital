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

#include <linux/input.h>

#include <QVector>

#include "keysequence.h"

static bool modFromString(const QStringRef &t, Qt::KeyboardModifiers *mod)
{
    if (t == QStringLiteral("ctrl")) {
        *mod = Qt::ControlModifier;
        return true;
    }
    if (t == QStringLiteral("alt")) {
        *mod = Qt::AltModifier;
        return true;
    }
    if (t == QStringLiteral("super") || t == QStringLiteral("meta") || t == QStringLiteral("win")) {
        *mod = Qt::MetaModifier;
        return true;
    }
    if (t == QStringLiteral("shift")) {
        *mod = Qt::ShiftModifier;
        return true;
    }
    return false;
}

struct Key {
    QString c;
    int k;
};

// The table is ordered!
static Key keymap[] = {
    { QStringLiteral("0"), KEY_0 },
    { QStringLiteral("1"), KEY_1 },
    { QStringLiteral("2"), KEY_2 },
    { QStringLiteral("3"), KEY_3 },
    { QStringLiteral("4"), KEY_4 },
    { QStringLiteral("5"), KEY_5 },
    { QStringLiteral("6"), KEY_6 },
    { QStringLiteral("7"), KEY_7 },
    { QStringLiteral("8"), KEY_8 },
    { QStringLiteral("9"), KEY_9 },
    { QStringLiteral("a"), KEY_A },
    { QStringLiteral("b"), KEY_B },
    { QStringLiteral("backspace"), KEY_BACKSPACE },
    { QStringLiteral("brightnessdown"), KEY_BRIGHTNESSDOWN },
    { QStringLiteral("brightnessup"), KEY_BRIGHTNESSUP },
    { QStringLiteral("c"), KEY_C },
    { QStringLiteral("d"), KEY_D },
    { QStringLiteral("down"), KEY_DOWN },
    { QStringLiteral("e"), KEY_E },
    { QStringLiteral("esc"), KEY_ESC },
    { QStringLiteral("f"), KEY_F },
    { QStringLiteral("f1"), KEY_F1 },
    { QStringLiteral("f2"), KEY_F2 },
    { QStringLiteral("f3"), KEY_F3 },
    { QStringLiteral("f4"), KEY_F4 },
    { QStringLiteral("f5"), KEY_F5 },
    { QStringLiteral("f6"), KEY_F6 },
    { QStringLiteral("f7"), KEY_F7 },
    { QStringLiteral("f8"), KEY_F8 },
    { QStringLiteral("f9"), KEY_F9 },
    { QStringLiteral("f10"), KEY_F10 },
    { QStringLiteral("f11"), KEY_F11 },
    { QStringLiteral("f12"), KEY_F12 },
    { QStringLiteral("g"), KEY_G },
    { QStringLiteral("h"), KEY_H },
    { QStringLiteral("i"), KEY_I },
    { QStringLiteral("j"), KEY_J },
    { QStringLiteral("k"), KEY_K },
    { QStringLiteral("l"), KEY_L },
    { QStringLiteral("left"), KEY_LEFT },
    { QStringLiteral("m"), KEY_M },
    { QStringLiteral("minus"), KEY_MINUS },
    { QStringLiteral("n"), KEY_N },
    { QStringLiteral("o"), KEY_O },
    { QStringLiteral("p"), KEY_P },
    { QStringLiteral("pagedown"), KEY_PAGEDOWN },
    { QStringLiteral("pageup"), KEY_PAGEUP },
    { QStringLiteral("printscreen"), KEY_SYSRQ },
    { QStringLiteral("q"), KEY_Q },
    { QStringLiteral("r"), KEY_R },
    { QStringLiteral("right"), KEY_RIGHT },
    { QStringLiteral("s"), KEY_S },
    { QStringLiteral("space"), KEY_SPACE },
    { QStringLiteral("t"), KEY_T },
    { QStringLiteral("v"), KEY_V },
    { QStringLiteral("volumedown"), KEY_VOLUMEDOWN },
    { QStringLiteral("volumeup"), KEY_VOLUMEUP },
    { QStringLiteral("u"), KEY_U },
    { QStringLiteral("up"), KEY_UP },
    { QStringLiteral("w"), KEY_W },
    { QStringLiteral("x"), KEY_X },
    { QStringLiteral("y"), KEY_Y },
    { QStringLiteral("z"), KEY_X },
};

static bool keyFromString(const QStringRef &t, int *key)
{
    int numkeys = sizeof(keymap) / sizeof(Key);
    int begin = 0, end = numkeys - 1;

    while (begin <= end) {
        int mid = (begin + end) / 2;
        const Key &k = keymap[mid];
        int c = t.compare(k.c);

        if (c < 0) {
            end = mid - 1;
        } else if (c > 0) {
            begin = mid + 1;
        } else {
            *key = k.k;
            return true;
        }
    }

    return false;
}


KeySequence::KeySequence(const QString &sequence)
           : m_valid(true)
           , m_mods(0)
           , m_key(0)
{
    const QString &lower = sequence.toLower();
    const QVector<QStringRef> &parts = lower.splitRef('+');
    for (const QStringRef &part: parts) {
        Qt::KeyboardModifiers m;
        if (modFromString(part, &m)) {
            m_mods |= m;
        } else if (!keyFromString(part, &m_key)) {
            qWarning("Failed to parse key sequence '%s'. Unknown token: '%s'.", qPrintable(sequence), qPrintable(part.toString()));
            m_valid = false;
            return;
        }
    }
}
