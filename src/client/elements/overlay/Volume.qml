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
import QtQuick.Window 2.1
import QtQuick.Controls 1.0
import Orbital 1.0
import QtGraphicalEffects 1.0


Rectangle {
    id: volume

    width: Screen.width / 4
    x: Screen.width / 2 - width / 2
    y: Screen.height * 4 / 5
    height: 30

    color: "dimgrey"
    radius: 4
    opacity: 0

    Behavior on opacity { PropertyAnimation { duration: 300 } }

    property variant control: Client.service("MixerService")

    RectangularGlow {
        id: effect
        anchors.fill: volume
        glowRadius: 2
        spread: 0
        color: "grey"
    }

    Timer {
        id: timer
        repeat: false
        interval: 2000

        onTriggered: volume.opacity = 0
    }

    ProgressBar {
        id: bar
        anchors.fill: parent

        anchors.leftMargin: 10
        anchors.rightMargin: anchors.leftMargin
        anchors.topMargin: 4
        anchors.bottomMargin: anchors.topMargin

        maximumValue: 100
        Binding { target: bar; property: "value"; value: control.muted ? 0 : control.master }
        Connections {
            target: control
            onMasterChanged: {
                volume.opacity = 1;
                timer.start();
            }
        }

        Behavior on value { SmoothedAnimation { velocity: 50 } }

        Text {
            anchors.centerIn: parent
            text: parent.value.toFixed(0) + "%"
            font.pixelSize: 3 * parent.height / 4
        }

    }
}
