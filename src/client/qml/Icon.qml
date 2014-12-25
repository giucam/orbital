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
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.0

Item {
    id: button

    width: 32
    height: width

    signal clicked()
    signal pressed()
    signal entered()
    property string icon: ""
    property alias cache: image.cache
    property int iconFillMode: Image.PreserveAspectFit
    readonly property bool containsMouse: mouseArea.containsMouse

    Item {
        id: icon
        anchors.fill: parent

        Image {
            id: image
            source: button.icon
            anchors.fill: parent
            sourceSize: Qt.size(32, 32)
            fillMode: button.iconFillMode
        }

        BrightnessContrast {
            id: glow
            anchors.fill: image
            source: image
            brightness: 0.3
            opacity: 0
            cached: true

            Behavior on opacity { PropertyAnimation {} }
        }

        states: [
            State {
                name: "released"
                PropertyChanges { target: icon; anchors.margins: 0 }
            },
            State {
                name: "pressed"
                when: mouseArea.pressed
                PropertyChanges { target: icon; anchors.margins: 1 }
            }
        ]

        transitions: Transition { NumberAnimation { properties: "anchors.margins"; duration: 80 } }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true

        onEntered: { button.entered(); glow.opacity = 1 }
        onExited: glow.opacity = 0
        onPressed: button.pressed()
        onClicked: { mouse.accepted = false; button.clicked() }
    }
}
