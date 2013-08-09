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
import Orbital 1.0

ElementConfig {
    id: config
    anchors.fill: parent

    property bool shown: Ui.configMode
    visible: opacity != 0
    opacity: shown ? 1 : 0

    Behavior on opacity { PropertyAnimation { } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onEntered: content.opacity = 1
        onExited: content.opacity = 0

        onPressed: {
            element.publish(Qt.point(mouse.x, mouse.y));
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.6
        }

        Item {
            id: content
            anchors.fill: parent

            Behavior on opacity { PropertyAnimation { } }

            Button {
                anchors.centerIn: parent
                width: 30
                height: 30
                icon: "image://icon/edit-delete"
                visible: opacity != 0

                onClicked: element.destroyElement()
            }
        }
    }

    Component.onCompleted: content.opacity = Ui.configMode //trick that works as long as you can't add new elements while the configMode is on
}
