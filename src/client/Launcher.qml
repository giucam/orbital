
import QtQuick 2.1
import QtGraphicalEffects 1.0

Item {
    id: launcher
    height: parent.height
    width: height

    property alias icon: image.source
    property string process

    Item {
        id: icon
        anchors.fill: parent

        Image {
            id: image
            anchors.fill: parent
            sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectFit
        }

        Glow {
            id: glow
            anchors.fill: image
            radius: 8
            samples: 16
            color: "white"
            source: image
            opacity: 0

            Behavior on opacity { PropertyAnimation {} }
        }

        states: [
            State {
                name: "released"
                PropertyChanges { target: icon; anchors.margins: 0 }
            },
            State {
                name: "pressed"
                when: mouseArea.pressed
                PropertyChanges { target: icon; anchors.margins: 1 }
            }
        ]

        transitions: Transition { NumberAnimation { properties: "anchors.margins"; duration: 80 } }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            ProcessLauncher.launch(process)
        }

        onEntered: glow.opacity = 0.5
        onExited: glow.opacity = 0
    }
}
