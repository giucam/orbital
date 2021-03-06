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
import QtQuick.Controls 1.0
import Orbital.MprisService 1.0

Item {
    id: item
    property variant window
    property variant screen
    property bool shrinked: false
    property bool actualShrinked: controls ? (mousearea.containsMouse ? false : shrinked) : shrinked

    Layout.minimumWidth: 10
    Layout.preferredWidth: actualShrinked ? height : 200
    Layout.minimumHeight: 10
    Layout.preferredHeight: actualShrinked ? width : 200

    property Item controls: null
    Mpris {
        id: mpris
        pid: window.pid

        onValidChanged: {
            if (valid) {
                var incubator = controlsComponent.incubateObject(style.item);
                if (incubator.status != Component.Ready) {
                    incubator.onStatusChanged = function(status) {
                        if (status == Component.Ready) {
                            controls = incubator.object;
                        }
                    }
                } else {
                    controls = incubator.object;
                }
            } else if (controls) {
                controls.destroy();
                controls = null;
            }
        }
    }

    Component {
        id: controlsComponent
        MouseArea {
            id: ma
            property int iconSize: 16
            property int spacing: 2
            width: iconSize * 4 + spacing * 3
            height: iconSize
            y: (parent.height - height) / 2.
            x: 25

            onClicked: {}
            Row {
                opacity: mousearea.containsMouse
                anchors.fill: parent
                spacing: parent.spacing
                Icon {
                    width: ma.iconSize
                    height: ma.iconSize
                    icon: "image://icon/media-skip-backward"
                    onClicked: mpris.previous()
                }
                Icon {
                    width: ma.iconSize
                    height: ma.iconSize
                    icon: mpris.playbackStatus == Mpris.Playing ? "image://icon/media-playback-pause" : "image://icon/media-playback-start"
                    onClicked: mpris.playPause()
                }
                Icon {
                    width: ma.iconSize
                    height: ma.iconSize
                    icon: "image://icon/media-playback-stop"
                    onClicked: mpris.stop()
                }
                Icon {
                    width: ma.iconSize
                    height: ma.iconSize
                    icon: "image://icon/media-skip-forward"
                    onClicked: mpris.next()
                }

                Behavior on opacity { PropertyAnimation {} }
            }
        }
    }

    property string title: (mpris.playbackStatus != Mpris.Stopped ? mpris.trackTitle : null) || window.title

    Timer {
        id: previewTimer
        interval: 200
        onTriggered: window.preview(item.screen)
    }

    MouseArea {
        id: mousearea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                var state = window.state;
                if (window.isMinimized()) {
                    state &= ~ Window.Minimized;
                } else if (window.isActive()) {
                    state |= Window.Minimized;
                }
                if (!window.isActive()) {
                    state |= Window.Active
                }
                window.setState(item.screen, state);
            } else if (mouse.button == Qt.MiddleButton) {
                item.shrinked = !item.shrinked;
            } else {
                menu.popup();
            }
        }
        onEntered: previewTimer.start()
        onExited: {
            if (previewTimer.running) {
                previewTimer.stop();
            } else {
                window.endPreview(item.screen);
            }
        }

        StyleItem {
            id: style
            component: CurrentStyle.taskBarItem
            anchors.fill: parent

            Binding { target: style.item; property: "title"; value: item.title }
            Binding { target: style.item; property: "icon"; value: window ? window.icon : "" }
            Binding { target: style.item; property: "state"; value: window.state }
        }

        Image {
            source: mpris.playbackStatus == Mpris.Playing ? "image://icon/media-playback-start" :
                    (mpris.playbackStatus == Mpris.Paused ? "image://icon/media-playback-pause" :
                    "image://icon/media-playback-stop")
            visible: mpris.valid
            width: 12
            height: 12
            sourceSize: Qt.size(16, 16)
            opacity: 0.8

            anchors.bottom: style.bottom
            anchors.margins: 2
            x: 2
        }

        Rectangle {
            width: mpris.trackLength ? parent.width * mpris.trackPosition / mpris.trackLength : 0
            height: 10
            y: parent.height - 0.5
            color: "#C12E78"
        }
    }

    Menu {
        id: menu

        MenuItem {
            text: qsTr("Close")
            onTriggered: window.close();
        }

    }
}
