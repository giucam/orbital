
import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Layouts 1.0
import Orbital 1.0

ShellItem {
    id: panel
    type: ShellItem.Panel
    width: Screen.width
    height: 30
    color: "black"

    property Item content: layout

    Rectangle {
        parent: panel.contentItem
        anchors.fill: parent

        gradient: Gradient {
            GradientStop { position: 1.0; color: "black" }
            GradientStop { position: 0.0; color: "dimgrey" }
        }

        RowLayout {
            id: layout
            anchors.fill: parent
            anchors.margins: 2
        }
    }

}
