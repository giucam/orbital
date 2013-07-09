
import QtQuick 2.1
import QtQuick.Window 2.1

Rectangle {
    width: Screen.width
    height: Screen.height
    color: "black"

    Image {
        anchors.fill: parent
        source: "/home/giulio/Immagini/civetta/DSCF0470.JPG"
        fillMode: Image.PreserveAspectFit
        smooth: true
    }
}
