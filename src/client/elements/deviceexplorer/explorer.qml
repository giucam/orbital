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

PopupElement {
    id: explorer
    width: 250
    height: 300

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30

    minimumWidth: 100
    minimumHeight: 100

    Loader {
        id: loader
        source: "maincontent.qml"
        onStatusChanged: {
            if (status == Loader.Error) {
                source = "noservice.qml"
            }
        }
    }

    popupContent: loader.item
    popupWidth: 250
    popupHeight: 300
    buttonContent: Icon {
        id: icon
        anchors.fill: parent
        icon: "image://icon/drive-harddisk"

        onClicked: showPopup()
    }
}
