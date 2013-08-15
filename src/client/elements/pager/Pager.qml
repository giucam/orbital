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
    id: pager
    Layout.preferredWidth: grid.width + 5
    Layout.maximumWidth: 1000
    Layout.fillHeight: true

    width: Layout.preferredWidth
    height: 50

    contentItem: Grid {
        id: grid
        spacing: 2

        rows: Client.workspaces.length > 2 ? 2 : 1
        x: (pager.width - grid.width) / 2
        y: (pager.height - grid.height) / 2
        readonly property int cols: Client.workspaces.length > 0 ? Client.workspaces.length / rows : 1

        readonly property real ratio: Screen.height > 0 ? Screen.width / Screen.height : 1
        readonly property bool fitWidth: pager.width >= pager.height * ratio

        Repeater {
            model: Client.workspaces

            Item {
                height: grid.fitWidth ? pager.height / grid.rows : (pager.width / grid.cols) / grid.ratio
                width: height * grid.ratio

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
