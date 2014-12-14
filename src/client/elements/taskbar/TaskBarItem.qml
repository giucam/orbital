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
    property variant screen
    property bool shrinked: false

    Layout.minimumWidth: 10
    Layout.preferredWidth: shrinked ? height : 200
    Layout.minimumHeight: 10
    Layout.preferredHeight: shrinked ? width : 200

    StyleItem {
        id: style
        component: CurrentStyle.taskBarItem
        anchors.fill: parent

        Binding { target: style.item; property: "title"; value: window ? window.title : "" }
        Binding { target: style.item; property: "icon"; value: window ? window.icon : "" }
        Binding { target: style.item; property: "state"; value: window.state }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                var state = window.state;
                if (window.isMinimized()) {
                    state &= ~ Window.Minimized;
                } else if (window.isActive()) {
                    state |= Window.Minimized;
                }
                if (!window.isActive()) {
                    state |= Window.Active
                }
                window.setState(item.screen, state);
            } else if (mouse.button == Qt.MiddleButton) {
                item.shrinked = !item.shrinked;
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
