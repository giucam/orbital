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

    width: 32
    height: width

    Layout.preferredWidth: width
    Layout.maximumWidth: 50
    Layout.fillHeight: true

    contentItem: Button {
        anchors.fill: parent
        icon: launcher.icon

        onClicked: ProcessLauncher.launch(process)
    }

    toolTip: Text {
            width: 200
            height: 40
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: launcher.process
            color: launcher.style.textColor
            elide: Text.ElideMiddle
        }

    settingsItem: Rectangle {
        id: config
        width: 500
        height: 150

        Column {
            id: content
            anchors.fill: parent
            anchors.margins: 5
            property int middle: 60
            spacing: 3
            Item {
                width: parent.width
                height: 25

                Controls.Label {
                    width: content.middle - content.anchors.margins
                    height: parent.height
                    text: "Icon:"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }
                Controls.TextField {
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
                    width: content.middle - content.anchors.margins
                    height: parent.height
                    text: "Process:"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }
                Controls.TextField {
                    x: content.middle + content.spacing
                    width: content.width - content.anchors.margins - x
                    height: parent.height
                    text: launcher.process
                    onAccepted: launcher.process = text
                }
            }
        }
    }
}
