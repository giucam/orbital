
import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0

ShellItem {
    type: ShellItem.Background
    width: Screen.width
    height: Screen.height

    property alias imageSource: image.source
    property alias imageFillMode: image.fillMode

    Image {
        id: image
        anchors.fill: parent
        smooth: true
    }
}
