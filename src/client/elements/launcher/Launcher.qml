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
import QtQuick.Controls 1.0 as Controls
import QtGraphicalEffects 1.0
import Orbital 1.0

Element {
    id: launcher

    property string icon: "image://icon/image-missing"
    property string process

    saveProperties: [ "icon", "process" ]

    width: Layout.preferredWidth
    height: width

    Layout.preferredWidth: 32
    Layout.maximumWidth: 50
    Layout.preferredHeight: 32
    Layout.maximumHeight: 50

    property variant service: Client.service("ProcessLauncher")

    contentItem: Icon {
        anchors.fill: parent
        icon: launcher.icon

        onClicked: service.launch(process)
    }

    toolTip: Text {
            width: 200
            height: 40
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: launcher.process
            color: CurrentStyle.textColor
            elide: Text.ElideMiddle
        }

    settingsItem: Rectangle {
        id: config
        width: 500
        height: 150

        Column {
            id: content
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: buttons.top
            anchors.margins: 5
            property int middle: Math.max(iconLabel.width, commandLabel.width) + anchors.margins
            spacing: 3
            Item {
                width: parent.width
                height: 25

                Controls.Label {
                    id: iconLabel
                    x: content.middle - width
                    height: parent.height
                    text: qsTr("Icon:")
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }
                Controls.TextField {
                    id: iconText
                    x: content.middle + content.spacing
                    width: content.width - content.anchors.margins - x
                    height: parent.height
                    text: launcher.icon
                    onAccepted: launcher.icon = text
                }
            }

            Item {
                width: parent.width
                height: 25

                Controls.Label {
                    id: commandLabel
                    x: content.middle - width
                    height: parent.height
                    text: qsTr("Process:")
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }
                Controls.TextField {
                    id: commandText
                    x: content.middle + content.spacing
                    width: content.width - content.anchors.margins - x
                    height: parent.height
                    text: launcher.process
                    onAccepted: launcher.process = text
                }
            }
        }
        Row {
            id: buttons
            anchors.margins: 5
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 5
            Controls.Button {
                width: 100
                height: 30
                text: qsTr("Ok")
                onClicked: {
                    launcher.process = commandText.text;
                    launcher.icon = iconText.text;
                    launcher.closeSettings();
                }
            }
            Controls.Button {
                width: 100
                height: 30
                text: qsTr("Cancel")
                onClicked: {
                    commandText.text = launcher.process
                    iconText.text = launcher.icon
                    launcher.closeSettings();
                }
            }
        }
    }
}
