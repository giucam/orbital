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

Style {
    panelBackground: StyleComponent {
        topContentsMargin: 2
        leftContentsMargin: 2
        rightContentsMargin: 2
        bottomContentsMargin: 2
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 1.0; color: "black" }
                GradientStop { position: 0.0; color: "dimgrey" }
            }
        }
    }
    panelBorder: StyleComponent {
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
    }

    taskBarBackground: StyleComponent {
        topContentsMargin: 2
        leftContentsMargin: 2
        rightContentsMargin: 2
        bottomContentsMargin: 2
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "grey"
        }
    }

    taskBarItem: StyleComponent {
        property alias title: text.text
        property int state: Window.Inactive

        Rectangle {
            id: rect
            anchors.fill: parent
            color: "#3B3B37"

            property alias title: text.text
            property int state: Window.Inactive

            Text {
                id: text
                anchors.fill: parent
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                maximumLineCount: 1
                elide: Text.ElideRight
                color: "white"
            }

            states: [
                State {
                    name: "active"
                    when: state == Window.Active
                    PropertyChanges { target: rect; color: "dimgrey" }
                }
            ]
        }
    }

    pagerBackground: StyleComponent { }

    pagerWorkspace: StyleComponent {
        property bool active: false
        Rectangle {
            id: rect
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            border.color: active ? "white" : "grey"
        }
    }

    toolTipBackground: StyleComponent {
        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "#e6000000"
        }
    }

    textColor: "white"
    backgroundColor: "#E6404040"
}
