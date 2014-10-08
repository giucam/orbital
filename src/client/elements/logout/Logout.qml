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
import QtGraphicalEffects 1.0
import Orbital 1.0

Element {
    id: logout
    width: 100
    height: 32

    property int orientation: Layout ? Layout.orientation : Qt.Horizontal
    Layout.preferredWidth: height * 3 + 10
    Layout.preferredHeight: width * 3 + 10

    property variant service: Client.service("LoginService")

    contentItem: Layout {
        id: row
        orientation: logout.orientation
        anchors.fill: parent
        MouseArea {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.fillWidth: true
            Layout.fillHeight: true
            height: parent.height
            hoverEnabled: true
            onEntered: ttip.text = qsTr("Lock session")

            Icon {
                anchors.fill: parent
                icon: "image://icon/system-lock-screen"
                onClicked: service.lockSession()
            }
        }
        MouseArea {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.fillWidth: true
            Layout.fillHeight: true
            height: parent.height
            hoverEnabled: true
            onEntered: ttip.text = qsTr("Log out")

            Icon {
                anchors.fill: parent
                icon: "image://icon/system-log-out"
                onClicked: service.requestLogOut()
            }
        }
        MouseArea {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.fillWidth: true
            Layout.fillHeight: true
            height: parent.height
            hoverEnabled: true
            onEntered: ttip.text = qsTr("Shutdown")

            Icon {
                anchors.fill: parent
                icon: "image://icon/system-shutdown"
                onClicked: service.requestPoweroff()
            }
        }
        MouseArea {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.fillWidth: true
            Layout.fillHeight: true
            height: parent.height
            hoverEnabled: true
            onEntered: ttip.text = qsTr("Reboot")

            Icon {
                anchors.fill: parent
                icon: "image://icon/system-reboot"
                onClicked: service.requestReboot()
            }
        }
    }

    toolTip: Text {
        width: 100
        height: 40
        id: ttip
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        elide: Text.ElideMiddle
        color: CurrentStyle.textColor
    }
}
