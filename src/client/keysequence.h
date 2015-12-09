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

#ifndef ORBITAL_CLIENT_KEYSEQUENCE_H
#define ORBITAL_CLIENT_KEYSEQUENCE_H

#include <QString>

class KeySequence
{
public:
    explicit KeySequence(const QString &sequence);

    bool isValid() const { return m_valid; }
    Qt::KeyboardModifiers modifiers() const { return m_mods; }
    int key() const { return m_key; }

private:
    bool m_valid;
    Qt::KeyboardModifiers m_mods;
    int m_key;
};

#endif
