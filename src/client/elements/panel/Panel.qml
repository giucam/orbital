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
    id: panel
    property int position: 0
    property int orientation: (position == 0 || position == 2) ? Qt.Horizontal : Qt.Vertical
    property int size: 33

    width: orientation == Qt.Horizontal ? Screen.width : size
    height: orientation == Qt.Horizontal ? size : Screen.height
    inputRegion: background.childrenRect

    childrenConfig: Component {
        id: elementConfig

        ElementConfiguration {
        }
    }

    sortProperty: "layoutItem.index"
    childrenParent: layout

    onElementAdded: {
        element.parent = layout
    }
    onElementEntered: {
        var item = layout.childAt(pos.x, 15);
        if (item) {
            layout.insertAt(element, item.Layout.index);
            layout.relayout();
        } else {
            element.parent = layout;
        }
    }
    onElementMoved: {
        var item = layout.childAt(pos.x, 15);
        if (item) {
            if (item != element) {
                var index = item.Layout.index;
                if (pos.x < item.x + item.width - element.width)
                    index--;
                layout.insertAt(element, index);
            }
        }
    }
    onElementExited: {
        var newoffset = mapToItem(element, pos.x, pos.y);
        element.dragOffset = Qt.point(newoffset.x, newoffset.y);
    }

    contentItem: Item {
        anchors.fill: parent
        StyleItem {
            id: background
            anchors.fill: parent
            anchors.topMargin:    position == 2 ? 5 : 0
            anchors.leftMargin:   position == 3 ? 5 : 0
            anchors.bottomMargin: position == 0 ? 5 : 0
            anchors.rightMargin:  position == 1 ? 5 : 0
            component: CurrentStyle.panelBackground

            Layout {
                id: layout
                orientation: panel.orientation
                anchors.fill: parent
            }
        }
        StyleItem {
            id: border
            anchors.top:    panel.position == 0 ? background.bottom : contentItem.top
            anchors.bottom: panel.position == 2 ? background.top : contentItem.bottom
            anchors.left:   panel.position == 1 ? background.right : contentItem.left
            anchors.right:  panel.position == 3 ? background.left : contentItem.right
            component: CurrentStyle.panelBorder
            Binding { target: border.item; property: "panel"; value: panel }
        }
    }
}
