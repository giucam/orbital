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

Rectangle {
    id: mainContent
    color: Qt.rgba(CurrentStyle.backgroundColor.r, CurrentStyle.backgroundColor.g, CurrentStyle.backgroundColor.b, 0.6)
    anchors.fill: parent

    property variant service: Client.service("HardwareService")

    Component.onCompleted: {
        var devices = service.devices;
        for (var i = 0; i < devices.length; ++i) {
            var d = devices[i];
            if (d.type == Device.Storage) {
                devicesModel.append({ "device": devices[i] });
            }
        }
    }

    ListModel {
        id: devicesModel
    }

    Connections {
        target: service
        onDeviceAdded: {
            if (device.type == Device.Storage) {
                devicesModel.append({ "device": device });
            }
        }
        onDeviceRemoved: {
            if (device.type != Device.Storage) {
                return;
            }

            for (var i = 0; i < devicesModel.count; ++i) {
                if (devicesModel.get(i).device == device) {
                    devicesModel.remove(i);
                    return;
                }
            }
        }
    }

    ScrollView {
        id: view
        anchors.fill: parent
        ListView {
            id: listview
            anchors.fill: parent
            model: devicesModel

            property real h_y: 0
            property real h_width: 0
            property real h_height: 0
            property real h_opacity: 1

            highlight: Rectangle {
                color: CurrentStyle.highlightColor
                radius: 5
                opacity: listview.h_opacity
                y: listview.h_y
                width: listview.h_width
                height: listview.h_height
                Behavior on y { PropertyAnimation { duration: opacity > 0 ? 200 : 0 } }
                Behavior on opacity { PropertyAnimation { } }
            }
            highlightFollowsCurrentItem: false

            function setCurrentIndex(i) {
                if (i != -1) {
                    listview.currentIndex = i;
                    listview.h_y = listview.currentItem.y;
                    listview.h_width = listview.currentItem.width;
                    listview.h_height = listview.currentItem.height;
                }
                listview.h_opacity = i != -1;
            }

            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 200 }
            }
            add: Transition {
                NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 200 }
            }
            remove: Transition {
                NumberAnimation { properties: "opacity"; to: 0; duration: 200 }
            }

            delegate: MouseArea {
                id: device

                ListView.onRemove: SequentialAnimation {
                    PropertyAction { target: device; property: "ListView.delayRemove"; value: true }
                    NumberAnimation { target: device; property: "opacity"; to: 0; duration: 200 }
                    PropertyAction { target: device; property: "ListView.delayRemove"; value: false }
                }
                height: 30
                width: listview.width
                hoverEnabled: true
                onEntered: listview.setCurrentIndex(index)
                onExited: listview.setCurrentIndex(-1)
                Item {
                    anchors.fill: parent
                    anchors.margins: 2
                    Icon {
                        id: icon
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        icon: "image://icon/" + modelData.iconName
                    }
                    Text {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: icon.right
                        anchors.right: mount.left
                        verticalAlignment: Text.AlignVCenter
                        text: modelData.name
                    }
                    Icon {
                        id: mount
                        height: device.height / 2
                        y: height / 2
                        anchors.right: parent.right
                        icon: modelData.mounted ? "image://icon/media-eject" : "image://icon/media-playback-start"
                        onClicked: modelData.mounted ? modelData.umount() : modelData.mount()
                    }
                }
            }
        }
    }
}
