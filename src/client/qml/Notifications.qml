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
import Orbital.NotificationsService 1.0

Item {
    id: root

    Component {
        id: component
        NotificationWindow {
            id: notif
            inactive: true
            property QtObject notification
            property alias summary: summaryText.text
            property alias body: bodyText.text
            property alias icon: image.source

            StyleItem {
                id: content
                component: CurrentStyle.notificationBackground
                width: 300
                height: text.height + 2 * margin + item.topContentsMargin + item.bottomContentsMargin
                opacity: 0
                property int margin: 7

                Image {
                    id: image
                    width: 32
                    height: 32
                    fillMode: Image.PreserveAspectFit
                    anchors.left: parent.left
                    anchors.margins: content.margin
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize: Qt.size(32, 32)
                }
                Item {
                    id: text
                    anchors.left: image.right
                    anchors.right: parent.right
                    anchors.margins: content.margin
                    anchors.verticalCenter: parent.verticalCenter
                    height: Math.min(maxHeight, Math.max(40, summaryText.height + bodyText.height))
                    property int maxHeight: 150
                    Text {
                        id: summaryText
                        width: parent.width
                        height: summaryText.text ? Math.min(implicitHeight, text.maxHeight) : 0
                        font.bold: true
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        color: CurrentStyle.textColor
                    }
                    Text {
                        id: bodyText
                        anchors.top: summaryText.bottom
                        width: parent.width
                        height: Math.min(implicitHeight, text.maxHeight - summaryText.height) // Math.min(150, Math.max(40, implicitHeight))
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        color: CurrentStyle.textColor
                    }
                }

                NumberAnimation { target: content; property: "opacity"; to: 1; duration: 300; running: true }
                SequentialAnimation {
                    id: fadeOutAnim
                    NumberAnimation { target: content; property: "opacity"; to: 0; duration: 300 }
                    ScriptAction { script: notif.destroy() }
                }

                Connections {
                    target: notification
                    onExpired: fadeOutAnim.start()
                }
            }
        }
    }

    Connections {
        target: NotificationsManager
        onNotify: {
            component.createObject(root, { notification: notification, body: notification.body,
                                           summary: notification.summary,
                                           icon: "image://notifications/" + notification.id });
        }
    }
}