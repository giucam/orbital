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
import QtGraphicalEffects 1.0

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
        LinearGradient {
            id: gradient
            anchors.fill: parent
            start: location == 2 ? Qt.point(0, height) : (location == 3 ? Qt.point(width, 0) : Qt.point(0, 0))
            end: location == 1 ? Qt.point(width, 0) : (location == 0 || location == 4 ? Qt.point(0, height) : Qt.point(0, 0))
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
        property int horizontal: (location != 1 && location != 3)

        Rectangle {
            id: rect
            anchors.fill: parent
            color: "#3B3B37"

            property alias title: text.text
            property int state: Window.Inactive

            Rotator {
                anchors.fill: parent
                angle: horizontal ? 0 : (location == 1 ? 270 : 90)
                Text {
                    id: text
                    anchors.fill: parent
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: "white"
                }
            }

            Behavior on color { ColorAnimation { duration: 100 } }

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

            Behavior on border.color { ColorAnimation {} }
        }
    }

    toolTipBackground: StyleComponent {
        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "#e6000000"
        }
    }

    button: StyleComponent {
        property alias text: text.text
        property bool pressed: false

        Rectangle {
            anchors.fill: parent
            color: pressed ? "#505050" : "dimgrey"

            Behavior on color { ColorAnimation {} }

            Text {
                anchors.fill: parent
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                id: text
                color: "black"
            }
        }
    }

    popup: StyleComponent {
        clip: true
        topContentsMargin: location == 0 ? header.size : 0
        leftContentsMargin: location == 1 ? header.size : 0
        bottomContentsMargin: location == 2 ? header.size : 0
        rightContentsMargin: location == 3 ? header.size : 0
        property int horizontal: (location != 1 && location != 3)

        property alias header: title.text

        Rectangle {
            width: horizontal ? parent.width : parent.width + 10
            height: horizontal ? parent.height + 10 : parent.height
            y: location == 0 ? -10 : 0
            x: location == 1 ? -10 : 0
            radius: 3
            color: CurrentStyle.backgroundColor

            Rectangle {
                id: header
                property int size: 20
                color: "#5A0608"
                width: horizontal ? parent.width : header.size
                height: horizontal ? header.size : parent.height
                x: location == 1 ? 10 : (location == 3 ? parent.width - 30 : 0)
                y: location == 0 ? 10 : (location == 2 ? parent.height - 30 : 0)

                Rotator {
                    anchors.fill: parent
                    angle: horizontal ? 0 : (location == 1 ? 270 : 90)
                    Text {
                        id: title
                        color: "white"
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }

    popupLauncher: StyleComponent {
        property variant popup: null
        property int horizontal: (location != 1 && location != 3)

        clip: true
        Rectangle {
            width: horizontal ? parent.width : parent.width + 10
            height: horizontal ? parent.height + 10 : parent.height
            color: (popup && popup.visible) ? "#5A0608" : "transparent"
            radius: 3
            x: location == 3 ? -10 : 0
            y: location == 2 ? -10 : 0

            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    textColor: "white"
    backgroundColor: "#E6404040"
}
