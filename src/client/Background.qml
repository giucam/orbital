
import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0

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
}
