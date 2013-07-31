
import QtQuick 2.1
import QtQuick.Layouts 1.0

Item {
    Layout.minimumWidth: 10
    Layout.preferredWidth: 200
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

            Keys.onPressed: {
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    ProcessLauncher.launch(text.text)
                    text.text = ""
                }
            }
        }
    }
}
