
import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Controls 1.0
import Orbital 1.0

Item {
    id: item
    width: Screen.width
    height: Screen.height
    opacity: 0

    Behavior on opacity { PropertyAnimation {} }

    Timer {
        id: timer
        repeat: false
        interval: 2000

        onTriggered: item.opacity = 0
    }

    VolumeControl {
        id: control
    }

    ProgressBar {
        width: Screen.width / 4
        x: Screen.width / 2 - width / 2
        y: Screen.height * 4 / 5
        height: 30

        maximumValue: 100
        value: control.master
    }

    property variant volUp: Client.addKeyBinding(115, 0)
    Connections {
        target: volUp
        onTriggered: changeVol(+5)
    }

    property variant volDown: Client.addKeyBinding(114, 0)
    Connections {
        target: volDown
        onTriggered: changeVol(-5)
    }

    function changeVol(ch) {
        control.changeMaster(ch);
        item.opacity = 1;
        timer.restart();
    }
}
