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
    width: Screen.width
    height: Screen.height
    objectName: "overlay"

    readonly property bool hasContent: volume.visible || leave.visible

    onHasContentChanged: {
        if (hasContent) {
            showContent();
        } else {
            hideContent();
        }
    }

    contentItem: Item {
        anchors.fill: parent
        Volume { id: volume }
        Leave { id: leave}
    }
}
