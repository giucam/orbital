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
import QtQuick.Window 2.1
import Orbital 1.0

Element {
    Layout.preferredWidth: grid.width + 5
    Layout.maximumWidth: 1000
    Layout.fillHeight: true

    width: Layout.preferredWidth
    height: 50

    Grid {
        id: grid
        spacing: 2
        height: parent.height
        rows: Client.workspaces.length > 2 ? 2 : 1

        Repeater {
            model: Client.workspaces

            Item {
                height: grid.height / grid.rows - 1
                width: height * Screen.width / Screen.height

                Rectangle {
                    id: rect
                    anchors.fill: parent
                    color: "transparent"
                    border.color: modelData.active ? "white" : "grey"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: Client.selectWorkspace(modelData)
                }
            }
        }
    }
}
