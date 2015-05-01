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
            color: "#e6e6e6"
        }
    }

    panelBorder: StyleComponent {
        Rectangle {
            id: gradient
            anchors.fill: parent

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
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
        property string icon: ""
        property int state: Window.Inactive
        property int horizontal: (location != 1 && location != 3)

        Item {
            anchors.fill: parent

            Rectangle {
                id: rect
                color: "#BFD9F5"
                radius: 3
                height: horizontal ? parent.height + 10 : parent.height
                width: horizontal ? parent.width : parent.width + 10
                x: location == 3 ? -10 : 0
                y: location == 2 ? -10 : 0

                states: [
                    State {
                        name: "active"
                        when: item.state == Window.Active
                        PropertyChanges { target: rect; color: "#57A2FD" }
                    },
                    State {
                        name: "minimized"
                        when: item.state & Window.Minimized
                        PropertyChanges { target: rect; height: horizontal ? 10 : parent.height }
                        PropertyChanges { target: rect; width: horizontal ? parent.width : 10 }
                        PropertyChanges { target: rect; x: horizontal ? 0 : (location == 1 ? item.width - 3 : -5) }
                        PropertyChanges { target: rect; y: horizontal ? (location == 0 ? item.height - 3 : -5) : 0 }
                    }
                ]
                transitions: Transition {
                    NumberAnimation { properties: "x,y,width,height"; easing.type: Easing.InOutQuad }
                }

                Behavior on color { ColorAnimation { } }
            }

            Rotator {
                anchors.fill: parent
                angle: horizontal ? 0 : (location == 1 ? 270 : 90)
                Image {
                    id: icon
                    source: "image://icon/" + item.icon
                    x: 2
                    y: 2
                    width: Math.min(parent.width, parent.height) - 4
                    height: width
                    fillMode: Image.PreserveAspectFit
                    sourceSize: Qt.size(32, 32)
                }
                Text {
                    id: text
                    anchors.left: icon.right
                    anchors.leftMargin: 5
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    verticalAlignment: Qt.AlignVCenter
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: "black"
                }
            }
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
        property int _margin: head.size
        topContentsMargin: (location == 0 || location == 4) ? _margin : 0
        leftContentsMargin: location == 1 ? _margin : 0
        bottomContentsMargin: location == 2 ? _margin : 0
        rightContentsMargin: location == 3 ? _margin : 0
        property int horizontal: (location != 1 && location != 3)

        property alias header: title.text

        Rectangle {
            width: horizontal ? parent.width : parent.width + 10
            height: horizontal ? parent.height + 10 : parent.height
            y: (location == 0 || location == 4) ? -10 : 0
            x: location == 1 ? -10 : 0
            radius: 3
            color: CurrentStyle.backgroundColor

            Rectangle {
                id: head
                property int size: 20
                color: "#57A2FD"
                width: horizontal ? parent.width : head.size
                height: horizontal ? head.size : parent.height
                x: location == 1 ? 10 : (location == 3 ? parent.width - 30 : 0)
                y: (location == 0 || location == 4) ? 10 : (location == 2 ? parent.height - 30 : 0)

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

    notificationBackground: StyleComponent {
        id: item
        property real margin: 5
        topContentsMargin: margin
        leftContentsMargin: margin
        rightContentsMargin: margin
        bottomContentsMargin: margin

        Rectangle {
            id: content
            anchors.fill: parent
            anchors.margins: item.margin
            color: "#e6e6e6"
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: content.left
            anchors.right: content.right
            anchors.bottom: content.top

            gradient: Gradient {
                GradientStop { position: 1.0; color: "#f0202020" }
                GradientStop { position: 0.0; color: "#00000000" }
            }
        }
        Rectangle {
            anchors.top: content.bottom
            anchors.left: content.left
            anchors.right: content.right
            anchors.bottom: parent.bottom

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        Rotator {
            anchors.top: content.top
            anchors.left: parent.left
            anchors.right: content.left
            anchors.bottom: content.bottom
            angle: 90

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#f0202020" }
                    GradientStop { position: 1.0; color: "#00000000" }
                }
            }
        }
        Rotator {
            anchors.top: content.top
            anchors.left: content.right
            anchors.right: parent.right
            anchors.bottom: content.bottom
            angle: 90

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 1.0; color: "#f0202020" }
                    GradientStop { position: 0.0; color: "#00000000" }
                }
            }
        }
        RadialGradient {
            width: item.margin
            height: item.margin
            horizontalOffset: item.margin / 2
            verticalOffset: item.margin / 2

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        RadialGradient {
            width: item.margin
            height: item.margin
            horizontalOffset: -item.margin / 2
            verticalOffset: item.margin / 2
            x: parent.width - item.margin

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        RadialGradient {
            width: item.margin
            height: item.margin
            horizontalOffset: item.margin / 2
            verticalOffset: -item.margin / 2
            y: parent.height - item.margin

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        RadialGradient {
            width: item.margin
            height: item.margin
            horizontalOffset: -item.margin / 2
            verticalOffset: -item.margin / 2
            x: parent.width - item.margin
            y: parent.height - item.margin

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f0202020" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
    }

    textColor: "black"
    backgroundColor: "#F4F4F4"
    highlightColor: "#57A2FD"
}
