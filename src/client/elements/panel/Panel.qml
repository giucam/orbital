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
    property int orientation: (location == 0 || location == 2) ? Qt.Horizontal : Qt.Vertical
    property int size: 33

    width: (location == 0 || location == 2) ? Screen.width : size
    height: (location == 0 || location == 2) ? size : Screen.height

    property rect __rect: background.childrenRect
    property var __map: mapFromItem(background, __rect.x, __rect.y, __rect.width, __rect.height)
    inputRegion: Qt.rect(__map.x, __map.y, __map.width, __map.height)

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
        var item = null;
        if (orientation == Qt.Horizontal) {
            item = layout.childAt(pos.x, 15);
        } else {
            item = layout.childAt(15, pos.y);
        }
        if (item) {
            layout.insertAt(element, item.Layout.index);
            layout.relayout();
        } else {
            element.parent = layout;
        }
    }
    onElementMoved: {
        var item = null;
        if (orientation == Qt.Horizontal) {
            item = layout.childAt(pos.x, 15);
        } else {
            item = layout.childAt(15, pos.y);
        }
        if (item) {
            if (item != element) {
                var index = item.Layout.index;
                if ((orientation == Qt.Horizontal && pos.x < item.x + item.width - element.width) ||
                    (pos.y < item.y + item.height - element.height))
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
            clip: true
            id: background
            anchors.fill: parent
            anchors.topMargin:    location == 2 ? 5 : 0
            anchors.leftMargin:   location == 3 ? 5 : 0
            anchors.bottomMargin: location == 0 ? 5 : 0
            anchors.rightMargin:  location == 1 ? 5 : 0
            component: CurrentStyle.panelBackground

            Item {
                id: handle
                visible: Ui.configMode
                anchors.top: parent.top
                anchors.left: parent.left
                width: orientation == Qt.Horizontal ? (visible ? parent.height : 0) : parent.width
                height: orientation == Qt.Vertical ? (visible ? parent.width : 0) : parent.height

                Icon {
                    anchors.fill: parent
                    // This is in the oxygen theme but not in the xdg icon naming spec.
                    // i'd really like to use one in the spec but i couldn't find a suitable one.
                    icon: "image://icon/transform-move"
                    onPressed: {
                        panel.publish(Qt.point(0, 0));
                    }
                }

                Behavior on width { PropertyAnimation {} }
                Behavior on height { PropertyAnimation {} }
            }

            Layout {
                id: layout
                orientation: panel.orientation
                anchors.top: panel.orientation == Qt.Horizontal ? parent.top : handle.bottom
                anchors.left: panel.orientation == Qt.Horizontal ? handle.right : parent.left
                anchors.bottom: parent.bottom
                anchors.right: parent.right
            }
        }
        StyleItem {
            id: border
            anchors.top:    panel.location == 0 ? background.bottom : contentItem.top
            anchors.bottom: panel.location == 2 ? background.top : contentItem.bottom
            anchors.left:   panel.location == 1 ? background.right : contentItem.left
            anchors.right:  panel.location == 3 ? background.left : contentItem.right
            component: CurrentStyle.panelBorder
        }
    }
}
