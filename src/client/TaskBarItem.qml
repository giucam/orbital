
import QtQuick 2.1
import QtQuick.Layouts 1.0

Rectangle {
    id: item
    color: "#3B3B37"
    property variant window

    Layout.minimumWidth: 10
    Layout.preferredWidth: text.width + 20
    Layout.maximumWidth: 250
    Layout.fillHeight: true

    Text {
        id: text
        anchors.centerIn: parent
        color: "white"
        text: window ? window.title : ""
    }

    states: [
        State {
            name: "active"
            when: window.active
            PropertyChanges { target: item; color: "dimgrey" }
        }
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (window.active) {
                window.minimize();
            } else {
                window.activate();
            }
        }
    }

}
