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
import QtGraphicalEffects 1.0

ElementConfig {
    id: config
    anchors.fill: parent

    property bool shown: Ui.configMode
    visible: opacity != 0
    opacity: shown ? 1 : 0

    Behavior on opacity { PropertyAnimation { } }

    onChildrenChanged: {
        for (var i = 0; i < children.length; ++i) {
            var c = children[i];
            if (c != mouseArea) {
                c.parent = mouseArea;
            }
        }
    }



    MouseArea {
        id: mouseArea
        anchors.fill: parent

        onPressed: {
            element.publish(Qt.point(mouse.x, mouse.y));
        }

        FastBlur {
            id: glow
            anchors.fill: parent
            radius: 8
            source: element.content
            cached: true
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.2
        }

        Item {
            id: content
            anchors.fill: parent

            Button {
                id: deleteButton
                anchors.top: parent.top
                anchors.left: parent.left

                width: 17
                height: width
                icon: "image://icon/edit-delete"

                onClicked: element.destroyElement()
            }
            Button {
                id: configureButton
                anchors.top: parent.top
                anchors.left: deleteButton.right
                anchors.leftMargin: 1

                width: 15
                height: width
                icon: "image://icon/preferences-other"

                onClicked: element.configure()
            }
        }
    }

    Component.onCompleted: configureButton.visible = element.settingsItem != null
}
