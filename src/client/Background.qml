
import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Controls 1.0 as Controls

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

    FileDialog {
        id: fileDialog
        title: "Please choose an image file"
        nameFilters: [ "All files (*)" ]
        selectedNameFilter: nameFilters[0]
        onAccepted: {
            bkg.imageSource = fileDialog.fileUrls[0];
        }
    }

    Rectangle {
        id: config
        y: bkg.height
        width: parent.width
        height: 200
        color: "dimgrey"

        property bool open: false
        states: [
            State {
                name: "open"
                when: config.open
                PropertyChanges { target: config; y: bkg.height - config.height }
            }
        ]

        Row {
            Text {
                text: image.source
            }

            Button {
                icon: "image://icon/configure"
                onClicked: fileDialog.open()
            }

        }

        Button {
            anchors.bottom: config.bottom
            x: config.width - configButton.width - width
            width: configButton.width
            icon: "image://icon/dialog-cancel"

            onClicked: Ui.reloadConfig()
        }

        Behavior on y { PropertyAnimation { } }
    }

    Button {
        id: configButton
        width: 20
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        icon: "image://icon/configure"

        onClicked:  {
            if (config.open) {
                Ui.saveConfig();
            }
            config.open = !config.open;
        }
    }
}
