/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.0

Item {
    id: root
    width: 450
    height: 300
    focus: true

    Keys.onEscapePressed: Qt.quit()

    Image {
        id: image
        anchors.bottom: uploadStatus.top
        anchors.margins: 7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        cache: false
        fillMode: Image.PreserveAspectFit
    }

    TextArea {
        id: uploadStatus
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttons.top
        anchors.margins: 7
        height: 50
        readOnly: true
    }

    Row {
        id: buttons
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.margins: 7
        spacing: 5

        Button {
            text: "New shot"
            height: 30
            width: 100

            onClicked: Screenshooter.takeShot()
        }
        Button {
            text: "New surface shot"
            height: 30
            width: 100

            onClicked: Screenshooter.takeSurfaceShot()
        }
        Button {
            text: "Save as"
            height: 30
            width: 100

            onClicked: fileDialog.open()
        }
        Button {
            text: "Upload to Imgur"
            height: 30
            width: 100

            onClicked: Screenshooter.upload()
        }
    }

    FileDialog {
        id: fileDialog
        title: "Save as"
        selectExisting: false
        nameFilters: [ "Image files (*.jpg *.png)", "All files (*)" ]
        onAccepted: Screenshooter.save(fileDialog.fileUrl)
    }

    Connections {
        target: Screenshooter
        onNewShot: {
            image.source = "";
            image.source = "image://screenshoter/shot";
        }
        onUploadOutput: {
            uploadStatus.text = output;
        }
    }
}
