
import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Controls 1.0
import Orbital 1.0
import QtGraphicalEffects 1.0


Rectangle {
    id: volume

    width: Screen.width / 4
    x: Screen.width / 2 - width / 2
    y: Screen.height * 4 / 5
    height: 30

    color: "dimgrey"
    radius: 4
    opacity: 0

    Behavior on opacity { PropertyAnimation { duration: 300 } }

    RectangularGlow {
        id: effect
        anchors.fill: volume
        glowRadius: 2
        spread: 0
        color: "grey"
    }

    Timer {
        id: timer
        repeat: false
        interval: 2000

        onTriggered: volume.opacity = 0
    }

    VolumeControl {
        id: control
    }

    ProgressBar {
        anchors.fill: parent

        anchors.leftMargin: 10
        anchors.rightMargin: anchors.leftMargin
        anchors.topMargin: 4
        anchors.bottomMargin: anchors.topMargin

        maximumValue: 100
        value: control.master

        Behavior on value { SmoothedAnimation { velocity: 50 } }

        Text {
            anchors.centerIn: parent
            text: parent.value.toFixed(0) + "%"
            font.pixelSize: 3 * parent.height / 4
        }

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
        volume.opacity = 1;
        timer.restart();
    }
}
