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

#include <QStringList>

#include "keysequence.h"

static bool modFromString(const QString &t, Qt::KeyboardModifiers *mod)
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

static Key keymap[] = {
    { QStringLiteral("a"), KEY_A },
    { QStringLiteral("b"), KEY_B },
    { QStringLiteral("c"), KEY_C },
    { QStringLiteral("d"), KEY_D },
    { QStringLiteral("e"), KEY_E },
    { QStringLiteral("f"), KEY_F },
    { QStringLiteral("g"), KEY_G },
    { QStringLiteral("h"), KEY_H },
    { QStringLiteral("i"), KEY_I },
    { QStringLiteral("j"), KEY_J },
    { QStringLiteral("k"), KEY_K },
    { QStringLiteral("l"), KEY_L },
    { QStringLiteral("m"), KEY_M },
    { QStringLiteral("n"), KEY_N },
    { QStringLiteral("o"), KEY_O },
    { QStringLiteral("p"), KEY_P },
    { QStringLiteral("q"), KEY_Q },
    { QStringLiteral("r"), KEY_R },
    { QStringLiteral("s"), KEY_S },
    { QStringLiteral("t"), KEY_T },
    { QStringLiteral("v"), KEY_V },
    { QStringLiteral("u"), KEY_U },
    { QStringLiteral("w"), KEY_W },
    { QStringLiteral("x"), KEY_X },
    { QStringLiteral("y"), KEY_Y },
    { QStringLiteral("z"), KEY_X },
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
    { QStringLiteral("left"), KEY_LEFT },
    { QStringLiteral("right"), KEY_RIGHT },
    { QStringLiteral("up"), KEY_UP },
    { QStringLiteral("down"), KEY_DOWN },
    { QStringLiteral("pageup"), KEY_PAGEUP },
    { QStringLiteral("pagedown"), KEY_PAGEDOWN },
    { QStringLiteral("esc"), KEY_ESC },
    { QStringLiteral("minus"), KEY_MINUS },
    { QStringLiteral("space"), KEY_SPACE },
    { QStringLiteral("backspace"), KEY_BACKSPACE },
    { QStringLiteral("volumeup"), KEY_VOLUMEUP },
    { QStringLiteral("volumedown"), KEY_VOLUMEDOWN },
    { QStringLiteral("printscreen"), KEY_SYSRQ },
};

static bool keyFromString(const QString &t, int *key)
{
    for (const Key &k: keymap) {
        if (k.c == t) {
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
    const QStringList &parts = sequence.toLower().split('+');
    for (const QString &part: parts) {
        Qt::KeyboardModifiers m;
        if (modFromString(part, &m)) {
            m_mods |= m;
        } else if (!keyFromString(part, &m_key)) {
            qWarning("Failed to parse key sequence '%s'. Unknown token: '%s'.", qPrintable(sequence), qPrintable(part));
            m_valid = false;
            return;
        }
    }
}
