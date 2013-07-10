
import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Layouts 1.0
import Orbital 1.0

ShellItem {
    type: ShellItem.Panel
    width: Screen.width
    height: 30
    color: "black"

    Rectangle {
        anchors.fill: parent

        gradient: Gradient {
            GradientStop { position: 1.0; color: "black" }
            GradientStop { position: 0.0; color: "dimgrey" }
        }

        RowLayout {
            anchors.fill: parent
            Launcher {
                icon: "/usr/share/icons/default.kde4/32x32/apps/utilities-terminal.png"
                process: "/home/giulio/projects/wayland/weston/clients/weston-terminal"
            }
        }
    }

}
