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

    readonly property real ratio: Screen.height > 0 ? Screen.width / Screen.height : 1

    Layout.preferredWidth: height * ratio
    Layout.preferredHeight: 50 / ratio

    width: Layout.preferredWidth
    height: 50

    contentItem: StyleItem {
        component: CurrentStyle.pagerBackground
        anchors.fill: parent
        Grid {
            width: parent.width
            height: parent.height
            id: grid

            rows: Client.workspaces.length > 2 ? 2 : 1
            readonly property int cols: Client.workspaces.length > 0 ? Client.workspaces.length / rows : 1

            readonly property bool fitWidth: width >= height * pager.ratio

            property int itemH: fitWidth ? height / rows : (width / cols) / pager.ratio
            property int itemW: itemH * pager.ratio

            x: (width - itemW * cols) / 2
            y: (height - itemH * rows) / 2

            Repeater {
                model: Client.workspaces

                Item {
                    height: grid.itemH
                    width: grid.itemW

                    StyleItem {
                        id: si
                        anchors.fill: parent
                        component: CurrentStyle.pagerWorkspace

                        Binding { target: si.item; property: "active"; value: modelData.active }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: Client.selectWorkspace(modelData)
                    }
                }
            }
        }
    }
}
