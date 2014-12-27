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

    property int baseLength: 30
    Layout.preferredWidth: (list.count ? list.count : 1) * baseLength + 6
    Layout.preferredHeight: Layout.preferredWidth

    width: horizontal ? Layout.preferredWidth : baseLength
    height: horizontal ? baseLength : Layout.preferredHeight
    property bool horizontal: true//location == 0 || location == 2 || location == 4

    property var service: Client.service("HardwareService")

    contentItem: ListView {
        id: list
        anchors.fill: parent
        orientation: root.horizontal ? ListView.Horizontal : ListView.Vertical
        model: service.batteries
        spacing: 3

        delegate: MouseArea {
            id: battery
            width: horizontal ? root.baseLength : list.width
            height: horizontal ? list.height : root.baseLength
            hoverEnabled: true
            property string iconName: "battery"
            property string tooltipText: ""

            Connections {
                target: modelData
                onChargeStateChanged: battery.updateTooltipText()
            }

            function updateTooltipText() {
                var state = ""
                if (modelData.chargeState == Battery.Charging) {
                    state = ": charging"
                } else if (modelData.chargeState == Battery.Discharging) {
                    state = ": discharging"
                }
                tooltipText = modelData.name + state
            }

            states: [
                State {
                    when: modelData.chargePercent < 40
                    PropertyChanges { target: battery; iconName: "battery-caution" }
                },
                State {
                    when: modelData.chargePercent < 20
                    PropertyChanges { target: battery; iconName: "battery-low" }
                }
            ]

            Icon {
                id: icon
                anchors.fill: parent
                icon: "image://icon/" + iconName
            }

            Text {
                anchors.fill: parent
                text: modelData.chargePercent + "%"
                styleColor: "lightgrey"
                style: Text.Outline
                font.weight: Font.Bold
                font.pointSize: 100
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
                minimumPointSize: 6
            }

            Binding {
                target: ttip
                property: "text"
                value: battery.tooltipText
                when: containsMouse
            }

            Component.onCompleted: battery.updateTooltipText()
        }

        MouseArea {
            id: noBattery
            width: horizontal ? root.baseLength : list.width
            height: horizontal ? list.height : root.baseLength
            hoverEnabled: true
            visible: list.count == 0

            Icon {
                anchors.fill: parent
                icon: "image://icon/battery-low"
            }
            Icon {
                anchors.fill: parent
                scale: 0.8
                icon: "image://icon/edit-delete"
            }

            Binding {
                target: ttip
                property: "text"
                value: "No battery"
                when: noBattery.containsMouse
            }
        }
    }

    toolTip: Text {
        width: 150
        height: 50
        id: ttip
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        wrapMode: Text.Wrap
        color: CurrentStyle.textColor
    }
}
