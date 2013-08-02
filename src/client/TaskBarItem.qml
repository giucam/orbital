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
import QtQuick.Layouts 1.0

Rectangle {
    id: item
    color: "#3B3B37"
    property variant window

    Layout.minimumWidth: 10
    Layout.preferredWidth: text.width + 20
    Layout.fillHeight: true

    Text {
        id: text
        anchors.centerIn: parent
        color: "white"
        text: window ? window.title : ""
    }

    states: [
        State {
            name: "active"
            when: window.active
            PropertyChanges { target: item; color: "dimgrey" }
        }
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (window.active) {
                window.minimize();
            } else {
                window.activate();
            }
        }
    }

}
