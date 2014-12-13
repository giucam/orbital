/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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
import Orbital 1.0

Element {
    id: root

    property int baseLength: 20
    Layout.preferredWidth: list.width
    Layout.preferredHeight: list.height

    width: Layout.preferredWidth
    height: Layout.preferredHeight
    property bool horizontal: location == 0 || location == 2 || location == 4

    property var service: Client.service("StatusNotifierService")

    contentItem: ListView {
        id: list
        width: horizontal ? count * root.baseLength + 6 : root.baseLength
        height: horizontal ? root.baseLength : count * root.baseLength + 6
        orientation: root.horizontal ? ListView.Horizontal : ListView.Vertical
        model: service.items
        spacing: 3

        delegate: MouseArea {
            width: root.baseLength
            height: root.baseLength
            hoverEnabled: true

            Icon {
                anchors.fill: parent
                icon: "image://icon/" + modelData.iconName
            }

            onClicked: {
                console.log("click");
                if (mouse.button == Qt.LeftButton) {
                    modelData.activate()
                } else if (mouse.button == Qt.MiddleButton) {
                    modelData.secondaryActivate()
                }
            }

            Binding {
                target: ttip
                property: "text"
                value: modelData.tooltipTitle
                when: containsMouse
            }
            onWheel: modelData.scroll(wheel.angleDelta)
        }
    }

    toolTip: Text {
        width: 200
        height: 70
        id: ttip
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        elide: Text.ElideMiddle
        color: CurrentStyle.textColor
    }
}
