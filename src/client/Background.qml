
import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0

ShellItem {
    type: ShellItem.Background
    width: Screen.width
    height: Screen.height

    Rectangle {
        anchors.fill: parent
        color: "black"

        Image {
            anchors.fill: parent
            source: "/home/giulio/Immagini/civetta/DSCF0470.JPG"
            fillMode: Image.PreserveAspectFit
            smooth: true
        }
    }
}
