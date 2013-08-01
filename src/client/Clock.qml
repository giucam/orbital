
import QtQuick 2.1
import QtQuick.Layouts 1.0

Item {
    width: text.width
    Layout.minimumWidth: width
    Layout.fillHeight: true

    Timer {
        interval: 1
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: text.text = Qt.formatDateTime(new Date(), "h:mm:ss")
    }

    Text {
        id: text
        anchors.centerIn: parent
        color: "white"
    }

}
