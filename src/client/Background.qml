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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0
import QtGraphicalEffects 1.0

ShellItem {
    id: bkg
    type: ShellItem.Background
    width: Screen.width
    height: Screen.height

    property string imageSource: ""
    property int imageFillMode: Image.PreserveAspectFit

    Image {
        id: image
        source: bkg.imageSource
        fillMode: bkg.imageFillMode
        anchors.fill: parent
        smooth: true
    }


    Rectangle {
        id: config
        y: bkg.height
        width: parent.width
        height: 300
        color: "#E6404040"

        property bool open: false
        property bool faded: false
        opacity: faded ? 0.1 : 1

        states: [
            State {
                name: "open"
                when: config.open
                PropertyChanges { target: config; y: bkg.height - config.height }
                StateChangeScript { script: browser.path = bkg.imageSource }
            }
        ]

        FileBrowser {
            id: browser
            nameFilters: [ "*.jpg", "*.png", "*.jpeg" ]
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            propagateComposedEvents: true
            onEntered: parent.faded = false
            onExited: parent.faded = true

            Button {
                id: goUp
                width: parent.width
                height: 20
                icon: "image://icon/go-up"

                onClicked: browser.cdUp()
            }

            Item {
                id: browserPanel
                anchors.top: goUp.bottom
                width: parent.width
                height: scrollBar.y + scrollBar.height
                Connections {
                    target: browser
                    onPathChanged: list.contentX = 0;
                }

                ListView {
                    id: list
                    spacing: 5
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: spacing
                    anchors.rightMargin: spacing
                    height: 100

                    Behavior on contentX { PropertyAnimation { easing.type: Easing.OutQuad } }

                    MouseArea {
                        anchors.fill: parent
                        propagateComposedEvents: true
                        onWheel: {
                            var pos = list.contentX + wheel.angleDelta.y * 2;
                            var max = list.contentWidth - parent.width;
                            var min = 0;
                            if (pos > max) pos = max;
                            if (pos < min) pos = min
                            list.contentX = pos;
                        }
                    }

                    model: browser.dirContent
                    orientation: ListView.Horizontal

                    delegate: Rectangle {
                        color: "black"
                        height: list.height
                        width: height

                        Item {
                            id: content
                            anchors.fill: parent
                            Image {
                                id: thumb
                                property int spacing: list.spacing
                                x: 2 * spacing
                                y: spacing
                                width: parent.width - 4 * spacing
                                height: width
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                cache: modelData.isDir()

                                source: modelData.isDir() ? "image://icon/folder" : modelData.path()
                            }
                            Text {
                                anchors.top: thumb.bottom
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                text: modelData.name
                                color: "white"
                                elide: Text.ElideMiddle
                            }
                        }

                        Glow {
                            id: glow
                            anchors.fill: content
                            radius: 8
                            samples: 16
                            color: "white"
                            source: content
                            opacity: 0

                            Behavior on opacity { PropertyAnimation {} }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true

                            onEntered: glow.opacity = 0.5
                            onExited: glow.opacity = 0

                            onClicked: {
                                if (modelData.isDir()) {
                                    browser.cd(modelData.name);
                                } else {
                                    bkg.imageSource = modelData.path();
                                }
                            }
                        }
                    }
                }

                Item {
                    id: scrollBar
                    anchors.top: list.bottom
                    anchors.left: list.left
                    anchors.right: list.right
                    anchors.topMargin: list.spacing
                    height: 15

                    Rectangle {
                        height: parent.height
                        width: 50
                        color: "black"
                        x: (parent.width - width) * list.contentX / (list.contentWidth - list.width)

                        MouseArea {
                            anchors.fill: parent
                            property int startPoint

                            onPressed: startPoint = mouse.x
                            onPositionChanged: {
                                var x = parent.x + mouse.x - startPoint;
                                if (x < 0) x = 0;
                                if (x > scrollBar.width - parent.width) x = scrollBar.width - parent.width;
                                list.contentX = x * (list.contentWidth - list.width) / (scrollBar.width - parent.width);
                            }
                        }
                    }
                }
            }

            Item {
                id: fillModeChooser
                anchors.top: browserPanel.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                anchors.topMargin: 5
                height: 20

                ListView {
                    id: fillModeList
                    anchors.fill: parent
                    model: [ "Scaled", "Scaled, keep aspect", "Scaled, cropped", "Centered", "Tiled" ]
                    orientation: ListView.Horizontal
                    spacing: 2
                    currentIndex: bkg.imageFillMode

                    delegate: Rectangle {
                        width: fillModeList.width / fillModeList.count - fillModeList.spacing
                        height: fillModeList.height
                        color: ListView.isCurrentItem ? "#505050" : "dimgrey"

                        Behavior on color { ColorAnimation {} }
                        Text {
                            text: modelData
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                bkg.imageFillMode = index;
                            }
                        }
                    }
                }
            }

            Button {
                anchors.bottom: parent.bottom
                x: config.width - configButton.width - width * 1.5
                width: configButton.width
                icon: "image://icon/document-revert"

                onClicked: Ui.reloadConfig()
            }
        }


        Behavior on opacity { PropertyAnimation { } }
        Behavior on y { PropertyAnimation { } }
    }

    MouseArea {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 20
        height:20

        hoverEnabled: true
        propagateComposedEvents: true
        onEntered: config.faded = false
        onExited: config.faded = true

        Button {
            id: configButton
            anchors.fill: parent

            icon: "image://icon/preferences-desktop-wallpaper"

            onClicked:  {
                if (config.open) {
                    Ui.saveConfig();
                }
                config.open = !config.open;
            }
        }
    }
}
