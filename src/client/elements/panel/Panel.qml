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
    width: Screen.width
    height: 33

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
        var offset = mapToItem(element, pos.x, pos.y);
        element.dragOffset = Qt.point(offset.x, offset.y);
    }

    contentItem: Item {
        anchors.fill: parent
        StyleItem {
            id: background
            anchors.fill: parent
            anchors.bottomMargin: 5
            component: style.panelBackground

            Layout {
                id: layout
                anchors.fill: parent
                anchors.margins: 2
            }
        }
        StyleItem {
            anchors.top: background.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            component: style.panelBorder
        }
    }
}
