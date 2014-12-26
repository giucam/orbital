/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

import QtQuick 2.2
import Orbital 1.0

Item {
    id: root
    property variant service: Client.service("NotificationsService")

    Component {
        id: component
        Notification {
            id: notification
            inactive: true
            property alias body: notificationText.text
            property alias icon: image.source

            StyleItem {
                id: content
                component: CurrentStyle.notificationBackground
                width: 300
                height: notificationText.height + 2 * margin + item.topContentsMargin + item.bottomContentsMargin
                opacity: 0
                property int margin: 7

                onHeightChanged: console.log(height)

                Image {
                    id: image
                    width: 32
                    height: 32
                    anchors.left: parent.left
                    anchors.margins: content.margin
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize: Qt.size(32, 32)
                }
                Text {
                    id: notificationText
                    anchors.left: image.right
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: content.margin
                    height: Math.min(150, Math.max(40, implicitHeight))
                    verticalAlignment: Qt.AlignVCenter
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    color: CurrentStyle.textColor
                }

                NumberAnimation { target: content; property: "opacity"; to: 1; duration: 300; running: true }
                SequentialAnimation {
                    id: fadeOutAnim
                    NumberAnimation { target: content; property: "opacity"; to: 0; duration: 300 }
                    ScriptAction { script: notification.destroy() }
                }

                Timer {
                    id: timer
                    interval: 5000
                    running: true
                    onTriggered: fadeOutAnim.start()
                }
            }
        }
    }

    Connections {
        target: service
        onNotify: {
            var text = "";
            if (summary)
                text += "<b>" + summary + "</b><br>";
            text += body;
            var notification = component.createObject(root, { body: text, icon: "image://icon/" + icon });
        }
    }
}