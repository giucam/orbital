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
import Orbital.HardwareService 1.0

Element {
    id: root

    property int baseLength: 30
    Layout.preferredWidth: (list.count ? list.count : 1) * baseLength + 6
    Layout.preferredHeight: Layout.preferredWidth

    width: horizontal ? Layout.preferredWidth : baseLength
    height: horizontal ? baseLength : Layout.preferredHeight
    property bool horizontal: true//location == 0 || location == 2 || location == 4

    contentItem: ListView {
        id: list
        anchors.fill: parent
        orientation: root.horizontal ? ListView.Horizontal : ListView.Vertical
        model: HardwareManager.batteries
        spacing: 3

        delegate: MouseArea {
            id: battery
            width: horizontal ? root.baseLength : list.width
            height: horizontal ? list.height : root.baseLength
            hoverEnabled: true
            property string iconName: "battery"
            property string tooltipText: ""

            property string charging: modelData.chargeState != Battery.Discharging ? "-charging" : ""
            property string charge: modelData.chargePercent < 20 ? "-low" : (modelData.chargePercent < 40 ? "-caution" : "")

            Connections {
                target: modelData
                onChargeStateChanged: battery.updateTooltipText()
            }

            function updateTooltipText() {
                var state = ""
                if (modelData.chargeState == Battery.Charging) {
                    state = ": charging"
                } else if (modelData.chargeState == Battery.Discharging) {
                    var secs = modelData.timeToEmpty;
                    var hours = Math.floor(secs / 3600);
                    secs -= hours * 3600;
                    hours = "" + hours + (hours == 1 ? " hour" : " hours");
                    var mins = Math.floor(secs / 60);
                    if (mins == 0) {
                        mins = "";
                    } else {
                        mins = (mins < 10 ? "0" + mins : mins);
                        mins = " and " + mins + (mins == 1 ? " minute" : " minutes");
                    }
                    state = ": discharging. Remaining time to empty: " + hours + mins + ".";
                }
                tooltipText = modelData.name + state
            }

            Icon {
                id: icon
                anchors.fill: parent
                icon: "image://icon/battery" + charging + charge
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
        width: 200
        height: 80
        id: ttip
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        wrapMode: Text.Wrap
        color: CurrentStyle.textColor
    }
}
