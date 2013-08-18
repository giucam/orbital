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
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.1
import Orbital 1.0

Element {
    width: 200
    height: 32

    Layout.minimumWidth: 10
    Layout.preferredWidth: width
    Layout.fillHeight: true

    contentItem: Rectangle {
        color: "white"
        anchors.fill: parent
        anchors.margins: 2

        TextInput {
            id: text
            anchors.fill: parent
            anchors.margins: 2
            verticalAlignment: TextInput.AlignVCenter

            onAccepted: {
                ProcessLauncher.launch(text.text)
                text.text = ""
            }
            Keys.onPressed: {
                if (event.key == Qt.Key_Escape) {
                    text.text = ""
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.IBeamCursor
            onPressed: {
                mouse.accepted = false;
            }
        }
    }
}
