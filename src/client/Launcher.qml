
import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.0

Item {
    id: launcher

    property string icon: ""
    property string process

    width: 32
    height: width

    Layout.preferredWidth: width
    Layout.maximumWidth: 50
    Layout.fillHeight: true

    Button {
        anchors.fill: parent
        icon: launcher.icon

        onClicked: {print("Click"); ProcessLauncher.launch(process)}
    }
}
