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
    Layout.preferredWidth: list.count * baseLength + 6
    Layout.preferredHeight: Layout.preferredWidth

    width: horizontal ? Layout.preferredWidth : baseLength
    height: horizontal ? baseLength : Layout.preferredHeight
    property bool horizontal: location == 0 || location == 2 || location == 4

    property var service: Client.service("StatusNotifierService")

    contentItem: ListView {
        id: list
        anchors.fill: parent
        orientation: root.horizontal ? ListView.Horizontal : ListView.Vertical
        model: service.items
        spacing: 3

        delegate: MouseArea {
            width: horizontal ? root.baseLength : list.width
            height: horizontal ? list.height : root.baseLength
            hoverEnabled: true
            property bool needsAttention: modelData.status == StatusNotifierItem.NeedsAttention
            property string iconName: needsAttention ? "attentionIcon" : "icon"

            Connections {
                target: modelData
                onIconChanged: if (iconName == "icon" ) icon.reloadIcon();
                onAttentionIconChanged: if (iconName == "attentionIcon" ) icon.reloadIcon();
            }

            Icon {
                id: icon
                anchors.fill: parent
                cache: false
                property bool reload: false

                function reloadIcon()
                {
                    reload = true;
                    reload = false;
                }

                icon: reload ? "" : "image://statusnotifier/" + modelData.service + "/" + iconName
            }

            onClicked: {
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
