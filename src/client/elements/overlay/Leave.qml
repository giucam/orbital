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

Item {
    id: leave
    anchors.fill: parent
    opacity: 0

    property variant service: Client.service("LoginService")
    property QtObject grab: null

    Behavior on opacity { PropertyAnimation { duration: 300 } }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "#80000000"
    }

    Text {
        id: text
        anchors.fill: parent
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignHCenter
        font.bold: true
        font.pointSize: 20
        font.capitalization: Font.SmallCaps
        style: Text.Outline
        styleColor: "grey"
        wrapMode: Text.Wrap

        property string op: ""
        text: "%1 in\n%2... \n\nClick anywhere to abort".arg(op).arg(service.timeout)
    }

    Connections {
        target: service
        onTimeoutStarted: {
            if (operation == "logout") {
                text.op = "Logging out";
            } else if (operation == "poweroff") {
                text.op = "Shutting down";
            } else if (operation == "reboot") {
                text.op = "Rebooting";
            }

            leave.opacity = 1;
            leave.grab = Client.createGrab();
        }
    }

    Connections {
        target: leave.grab
        onButton: {
            service.abortRequest();
            leave.grab.end();
            leave.grab.destroy();
            leave.opacity = 0;
        }
    }
}
