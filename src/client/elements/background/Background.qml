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
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.0

Element {
    id: bkg
    width: Screen.width
    height: Screen.height

    property variant fillModes: [ { name: "Scaled", value: 0 },
                                  { name: "Scaled, keep aspect", value: 1},
                                  { name: "Scaled, cropped", value: 2 },
                                  { name: "Centered", value: 6 },
                                  { name: "Tiled", value: 3 } ]

    property string imageSource: ""
    property int imageFillMode: 1
    property color color: "black"

    childrenParent: contentItem
    childrenConfig: Component {
        id: elementConfig

        ElementConfiguration {
            property int margin: 5
            property int resizeEdge: 0
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                property bool resizing: false
                property point resizeStart

                onPositionChanged: {
                    if (resizing) {
                        var mx = mouse.x;
                        var my = mouse.y;

                        if (resizeEdge & Qt.LeftEdge) {
                            var diff = mouse.x - resizeStart.x;
                            element.width -= diff;
                            element.x += diff;
                            mx -= diff;
                        }
                        if (resizeEdge & Qt.RightEdge)
                            element.width += mouse.x - resizeStart.x;
                        if (resizeEdge & Qt.TopEdge) {
                            var diff = mouse.y - resizeStart.y;
                            element.height -= diff;
                            element.y += diff;
                            my -= diff;
                        }
                        if (resizeEdge & Qt.BottomEdge)
                            element.height += mouse.y - resizeStart.y;

                        resizeStart = Qt.point(mx, my);

                        return;
                    }

                    resizeEdge = 0;
                    if (mouse.x < margin) resizeEdge |= Qt.LeftEdge;
                    if (mouse.y < margin) resizeEdge |= Qt.TopEdge;
                    if (mouse.x > width - margin) resizeEdge |= Qt.RightEdge;
                    if (mouse.y > height - margin) resizeEdge |= Qt.BottomEdge;

                    var cursorShape;
                    switch (resizeEdge) {
                        case Qt.RightEdge:
                        case Qt.LeftEdge:
                            cursorShape = Qt.SizeHorCursor; break;
                        case Qt.BottomEdge:
                        case Qt.TopEdge:
                            cursorShape = Qt.SizeVerCursor; break;
                        case Qt.LeftEdge | Qt.TopEdge:
                        case Qt.BottomEdge | Qt.RightEdge:
                            cursorShape = Qt.SizeFDiagCursor; break;
                        case Qt.LeftEdge | Qt.BottomEdge:
                        case Qt.RightEdge | Qt.TopEdge:
                            cursorShape = Qt.SizeBDiagCursor; break;
                        default:
                            Ui.restoreOverrideCursorShape();
                            return;
                    }
                    Ui.setOverrideCursorShape(cursorShape);
                }
                onExited: {
                    if (!resizing)
                        Ui.restoreOverrideCursorShape();
                }

                onPressed: {
                    mouse.accepted = mouse.button == Qt.LeftButton && resizeEdge != 0;
                    resizing = mouse.accepted;
                    if (resizing) {
                        resizeStart = Qt.point(mouse.x, mouse.y);
                    }
                }
                onReleased: {
                    resizing = false;
                    if (!containsMouse)
                        Ui.restoreOverrideCursorShape();
                }
            }
        }
    }

    childrenBackground: Component {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -5
            color: "#60606060"
            border.color: "dimgrey"
        }
    }

    onElementAdded: {
        element.addProperty("x");
        element.addProperty("y");
        element.addProperty("width");
        element.addProperty("height");

        var pos = mapToItem(config, pos.x, pos.y);
        if (pos.x >= 0 && pos.y >= 0 && pos.x <= config.width && pos.y <= config.height)
            config.faded = false;
    }
    onElementEntered: {
        config.faded = true;
        element.parent = bkg;
    }
    onElementMoved: {
        element.x = pos.x - offset.x;
        element.y = pos.y - offset.y;
    }
    onElementExited: {
    }

    contentItem: Rectangle {
        anchors.fill: parent
        color: bkg.color

        Image {
            id: image
            source: bkg.imageSource
            fillMode: bkg.fillModes[bkg.imageFillMode].value
            anchors.fill: parent
            smooth: true
            asynchronous: true
        }

        Menu {
            id: menu

            MenuItem {
                text: "Configure background"
                onTriggered: config.toggle();
            }
            MenuItem {
                text: Ui.configMode ? "Save configuration" : "Configure applets"
                onTriggered: Ui.toggleConfigMode();
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton

            onClicked: {
                if (!config.open) {
                    menu.popup();
                }
            }
        }

        Rectangle {
            id: config
            y: bkg.height
            width: parent.width
            height: 230
            color: CurrentStyle.backgroundColor
            z: 100
            visible: false

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

            function toggle() {
                if (config.open) {
                    Ui.saveConfig();
                    Client.restoreWindows();
                } else {
                    Client.minimizeWindows();
                }
                config.open = !config.open;
                Ui.configMode = false;
                config.visible = true;
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onEntered: parent.faded = false
                onExited: parent.faded = true

                Row {
                    id: buttons
                    width: parent.width
                    Icon {
                        id: home
                        height: 20
                        icon: "image://icon/user-home"

                        onClicked: browser.cdHome()
                    }
                    Icon {
                        id: goUp
                        height: 20
                        icon: "image://icon/go-up"

                        onClicked: browser.cdUp()
                    }
                }

                Item {
                    id: browserPanel
                    anchors.top: buttons.bottom
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
                                fast: true

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
                        model: bkg.fillModes
                        orientation: ListView.Horizontal
                        spacing: 2
                        currentIndex: bkg.imageFillMode

                        delegate: Item {
                            id: btn
                            width: fillModeList.width / fillModeList.count - fillModeList.spacing
                            height: fillModeList.height

                            property bool current: ListView.isCurrentItem

                            StyleItem {
                                id: style
                                anchors.fill: parent

                                component: CurrentStyle.button

                                Binding { target: style.item; property: "text"; value: modelData.name }
                                Binding { target: style.item; property: "pressed"; value: btn.current }
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

                ElementsChooser {
                    anchors.top: fillModeChooser.bottom
                    anchors.topMargin: 10
                    width: parent.width
                    height: 50
                }
            }


            Behavior on opacity { PropertyAnimation { } }
            Behavior on y { PropertyAnimation { } }
        }

        MouseArea {
            id: configButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 50
            height:20
            z: 101

            hoverEnabled: true
            propagateComposedEvents: true
            onEntered: config.faded = false
            onExited: config.faded = true

            Icon {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 20

                icon: "image://icon/preferences-desktop-wallpaper"

                onClicked: config.toggle()
            }

            Icon {
                id: revertButton
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 20
                icon: "image://icon/document-revert"

                onClicked: Ui.reloadConfig()
            }
        }

        Icon {
            anchors.bottom: configButtons.top
            anchors.right: parent.right
            anchors.bottomMargin: 10
            width: 20
            height: 20

            icon: "image://icon/preferences-desktop"

            onClicked: {
                Ui.toggleConfigMode();
            }
        }
    }
}
