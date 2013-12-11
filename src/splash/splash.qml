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

Rectangle {
    id: root
    color: "black"

    Item {
        id: orbiter
        anchors.centerIn: parent
        height: 200

        property real val: 0

        Rectangle {
            x: 150 * Math.cos(orbiter.val) - width / 2
            y: 100 * Math.sin(orbiter.val) - height / 2
            width: 20
            height: width
            color: "#575757"
            radius: width / 2
        }
        Rectangle {
            x: -width / 2
            y: -height / 2
            width: 50
            height: width
            color: "#BEBEBE"
            radius: width / 2
        }

        PropertyAnimation {
            target: orbiter
            property: "val"
            from: 0
            to: 2 * Math.PI
            loops: Animation.Infinite
            running: true
            duration: 1500
        }
    }

    Text {
        id: label
        text: "Loading Orbital..."
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: orbiter.bottom
        color: "white"
        font.pixelSize: 30
        font.bold: true

        property real val: 0
        opacity: val <= 1 ? val : 2 - val

        PropertyAnimation {
            target: label
            property: "val"
            from: 0
            to: 2
            loops: Animation.Infinite
            running: true
            duration: 1200
            easing.type: Easing.InOutQuad
        }
    }
}
