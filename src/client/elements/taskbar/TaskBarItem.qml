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

Item {
    id: item
    property variant window

    Layout.minimumWidth: 10
    Layout.preferredWidth: 200
    Layout.minimumHeight: 10
    Layout.preferredHeight: 200

    StyleItem {
        id: style
        component: CurrentStyle.taskBarItem
        anchors.fill: parent

        Binding { target: style.item; property: "title"; value: window ? window.title : "" }
        Binding { target: style.item; property: "state"; value: window.state }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                if (window.isMinimized()) {
                    window.unminimize();
                } else if (window.isActive()) {
                    window.minimize();
                }
                if (!window.isActive()) {
                    window.activate();
                }
            } else {
                menu.popup();
            }
        }
    }

    Menu {
        id: menu

        MenuItem {
            text: qsTr("Close")
            onTriggered: window.close();
        }

    }
}
