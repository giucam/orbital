/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Layouts 1.0
import Orbital 1.0

ShellItem {
    id: panel
    type: ShellItem.Panel
    width: Screen.width
    height: 30
    color: "black"

    property Item content: layout

    Rectangle {
        parent: panel.contentItem
        anchors.fill: parent

        gradient: Gradient {
            GradientStop { position: 1.0; color: "black" }
            GradientStop { position: 0.0; color: "dimgrey" }
        }

        RowLayout {
            id: layout
            anchors.fill: parent
            anchors.margins: 2
        }
    }

}
