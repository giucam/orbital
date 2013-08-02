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
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.0

Item {
    id: logout
    width: row.width
    height: 32

    Layout.preferredWidth: row.width
    Layout.maximumWidth: 100
    Layout.fillHeight: true

    Row {
        id: row
        height: parent.height
        Button {
            height: parent.height
            icon: "image://icon/system-log-out"
            onClicked: Client.logOut()
        }
        Button {
            height: parent.height
            icon: "image://icon/system-shutdown"
            onClicked: Client.poweroff()
        }
        Button {
            height: parent.height
            icon: "image://icon/system-reboot"
            onClicked: Client.reboot()
        }
    }
}
