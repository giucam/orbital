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

import QtQuick 2.2
import QtQuick.Window 2.2
import QtQuick.Controls 1.1
import Orbital 1.0

Element {
    width: Screen.width
    height: Screen.height

    property variant service: Client.service("LoginService")

    contentItem: Item {
        anchors.centerIn: parent

        TextField {
            id: password
            anchors.centerIn: parent
            placeholderText: qsTr("Password")
            width: 200
            height: 30
            enabled: !service.busy
            echoMode: TextInput.Password

            onAccepted: tryUnlock()

            ActiveRegion {
                anchors.fill: parent
            }
        }

        Button {
            anchors.horizontalCenter: password.horizontalCenter
            anchors.top: password.bottom
            anchors.margins: 5
            width: 100
            height: 30
            caption: qsTr("Unlock")
            enabled: !service.busy

            onClicked: tryUnlock()
        }
    }

    function tryUnlock() {
        service.tryUnlockSession(password.text, function(res) {
            password.text = "";
        })
    }

}
