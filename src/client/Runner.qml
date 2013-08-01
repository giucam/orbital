
import QtQuick 2.1
import QtQuick.Layouts 1.0

Item {
    width: 200
    height: 32

    Layout.minimumWidth: 10
    Layout.preferredWidth: width
    Layout.fillHeight: true

    Rectangle {
        color: "white"
        anchors.fill: parent
        anchors.margins: 2

        TextInput {
            id: text
            anchors.fill: parent
            anchors.margins: 2
            verticalAlignment: TextInput.AlignVCenter
            focus: true

            Keys.onPressed: {
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    ProcessLauncher.launch(text.text)
                    text.text = ""
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: Ui.requestFocus(text)
        }
    }
}
