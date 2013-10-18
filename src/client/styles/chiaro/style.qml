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
        Rectangle {
            anchors.fill: parent
            color: "#e6e6e6"
        }
    }

    panelBorder: StyleComponent {
        Component.onCompleted: update()
        onLocationChanged: update()
        LinearGradient {
            id: gradient
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        function update() {
            if (location == 1) {
                gradient.start = Qt.point(0, 0);
                gradient.end = Qt.point(gradient.width, 0);
            } else if (location == 2) {
                gradient.start = Qt.point(0, gradient.height);
                gradient.end = Qt.point(0, 0);
            } else if (location == 3) {
                gradient.start = Qt.point(gradient.width, 0);
                gradient.end = Qt.point(0, 0);
            } else {
                gradient.start = Qt.point(0, 0);
                gradient.end = Qt.point(0, gradient.height);
            }
        }
    }

    taskBarBackground: StyleComponent {
        Rectangle {
            anchors.fill: parent
            color: "transparent"
        }
    }

    taskBarItem: StyleComponent {
        id: item
        property alias title: text.text
        property int state: Window.Inactive
        property int horizontal: (location != 1 && location != 3)
        Item {
            anchors.fill: parent
            clip: true

            Rectangle {
                id: rect
                color: "#D2E3F5"
                radius: 3
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height + 10

                Behavior on color { ColorAnimation { duration: 100 } }
            }

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
                    color: "black"
                }
            }

            states: [
                State {
                    name: "active"
                    when: state == Window.Active
                    PropertyChanges { target: rect; color: "#57A2FD" }
                }
            ]
        }
    }

    pagerBackground: StyleComponent {
        bottomContentsMargin: 2
    }

    pagerWorkspace: StyleComponent {
        property bool active: false
        Rectangle {
            id: rect
            anchors.fill: parent
            anchors.margins: 1
            color: active ? "white" : "#B4B4B4"

            Behavior on border.color { ColorAnimation {} }
        }
    }

    toolTipBackground: StyleComponent {
        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "#e6e6e6e6"
        }
    }

    button: StyleComponent {
        property alias text: text.text
        property bool pressed: false

        Rectangle {
            anchors.fill: parent
            radius: 3
            border.color: "#a2a2a2"
            color: pressed ? "white" : "#c4c4c4"

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
                color: "#57A2FD"
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
            color: (popup && popup.visible) ? "#57A2FD" : "transparent"
            radius: 3
            x: location == 3 ? -10 : 0
            y: location == 2 ? -10 : 0

            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    textColor: "black"
    backgroundColor: "#F4F4F4"
}
