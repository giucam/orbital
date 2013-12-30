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
import QtQuick.Controls 1.0

Rectangle {
    id: noserviceContent
    color: Qt.rgba(CurrentStyle.backgroundColor.r, CurrentStyle.backgroundColor.g, CurrentStyle.backgroundColor.b, 0.6)
    anchors.fill: parent
    Text {
        text: qsTr("Error: unable to load the \"HardwareService\" service. Check your installation.")
        anchors.fill: parent
        anchors.margins: 5
        wrapMode: Text.Wrap
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
