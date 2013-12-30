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

Element {
    id: explorer
    width: 250
    height: 300

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30

    property variant service: Client.service("HardwareService")

    Component.onCompleted: {
        explorer.update();
    }

    property int _minWidth: 100
    property int _minHeight: 100

    function update() {
        if (explorer.width < _minWidth || explorer.height < _minHeight) {
            loader.item.parent = popup.content;
            explorer.contentItem = popupButton;
        } else {
            popupButton.parent = null;
            explorer.contentItem = loader.item;
        }
    }
    onWidthChanged: update()
    onHeightChanged: update()


    StyleItem {
        id: popupButton
        anchors.fill: parent
        component: CurrentStyle.popupLauncher

        Binding { target: popupButton.item; property: "popup"; value: popup }

        Icon {
            id: icon
            anchors.fill: parent
            icon: "image://icon/drive-harddisk"

            onClicked: popup.show()
        }
    }

    Popup {
        id: popup
        parentItem: explorer

        content: StyleItem {
            id: style

            width: 250
            height: 300

            component: CurrentStyle.popup
            Binding { target: style.item; property: "header"; value: explorer.prettyName }
        }
    }

    Loader {
        id: loader
        source: service ? "maincontent.qml" : "noservice.qml"
    }
}
