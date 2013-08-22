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
import QtQuick.Controls 1.0 as Controls
import QtGraphicalEffects 1.0
import Orbital 1.0

ElementBase {
    property alias toolTip: tt.content

    content: MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        ToolTip {
            id: tooltipWindow
            anchors.fill: parent

            content: StyleItem {
                id: tt
                component: style.toolTipBackground
                property Item content: null

                Binding { target: tt.content; property: "parent"; value: tt }
                Binding { target: tt; property: "width"; value: tt.content ? tt.content.width: 0 }
                Binding { target: tt; property: "height"; value: tt.content ? tt.content.height: 0 }
            }
        }

        onEntered: {
            tooltipWindow.show();
        }
        onExited: {
            tooltipWindow.hide();
        }
    }
}
