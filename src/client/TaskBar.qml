
import QtQuick 2.1
import QtQuick.Layouts 1.0

Rectangle {
    id: taskbar
    color: "transparent"
    border.color: "grey"

    width: 200
    height: 32

    Layout.preferredWidth: height
    Layout.preferredHeight: width
    Layout.fillWidth: true
    Layout.fillHeight: true

    RowLayout {
        id: layout
        spacing: 5
        anchors.fill: parent
        anchors.margins: 2

        Repeater {
            model: Client.windows

            TaskBarItem {
                window: modelData
            }

            onItemAdded: {
                print(item.parent)
                spacer.parent = null;
                spacer.parent = layout;
            }
        }

        Spacer { id: spacer }
    }
}
