/*
 * Copyright 2015 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.0
import Orbital 1.0
import QtQuick.Window 2.1
import QtQuick.Controls 1.0

PopupElement {
    id: root
    width: 50
    height: 150

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30
    minimumWidth: 40
    minimumHeight: 40
    property int selectionActive: -1

    buttonContent: Icon {
        id: icon
        anchors.fill: parent
        icon: "image://icon/edit-paste";

        onClicked: showPopup()

    }

    Clipboard.onTextChanged: {
        var curr = historyModel.count > 0 ? historyModel.get(0).text : ""
        var text = Clipboard.text;
        console.log("text",text,curr)
        if (!text) {
            selectionActive = -1;
            return;
        }
        selectionActive = 0;
        if (text == curr) {
            return;
        }

        for (var i = 0; i < historyModel.count; ++i) {
            if (historyModel.get(i).text == text) {
                historyModel.remove(i);
                break;
            }
        }

        historyModel.insert(0, { "text": text });
        if (historyModel.count > 15) {
            historyModel.remove(14);
        }
    }

    function activate(index) {
        if (index == selectionActive) {
            return;
        }
        var text = historyModel.get(index).text;
        Clipboard.text = text
    }

    ListModel {
        id: historyModel
    }

    popupWidth: 300
    popupHeight: 350
    popupContent: MouseArea {
        id: mousearea
        anchors.fill: parent
        hoverEnabled: true

        onPositionChanged: {
            listview.updateCurrentItem();
        }
        onExited: {
            listview.setCurrentIndex(-1);
        }

        ListView {
            id: listview
            anchors.fill: parent
            model: historyModel
            clip: true

            property real h_y: 0
            property real h_width: 0
            property real h_height: 0
            property real h_opacity: 0

            highlight: Rectangle {
                color: CurrentStyle.highlightColor
                radius: 5
                opacity: listview.h_opacity
                y: listview.h_y
                width: listview.h_width
                height: listview.h_height
                Behavior on opacity { PropertyAnimation { } }
                Behavior on y { PropertyAnimation { duration: opacity > 0 ? 200 : 0 } }
                Behavior on height { PropertyAnimation { duration: opacity > 0 ? 200 : 0 } }
            }
            highlightFollowsCurrentItem: false

            function updateCurrentItem() {
                if (mousearea.containsMouse) {
                    setCurrentIndex(indexAt(contentX + mousearea.mouseX, contentY + mousearea.mouseY));
                }
            }
            function setCurrentIndex(i) {
                if (i != -1) {
                    listview.currentIndex = i;
                    listview.h_y = listview.currentItem.y;
                    listview.h_width = listview.currentItem.width;
                    listview.h_height = listview.currentItem.height;
                }
                listview.h_opacity = i != -1;
            }

            displaced: Transition {
                id: trans
                SequentialAnimation {
                    NumberAnimation { properties: "x,y"; duration: 200 }
                    ScriptAction {
                        script: {
                            listview.updateCurrentItem();
                        }
                    }
                }
            }
            add: Transition {
                NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 200 }
            }
            remove: Transition {
                NumberAnimation { properties: "opacity"; to: 0; duration: 200 }
            }

            delegate: MouseArea {
                onClicked: root.activate(index)
                height: text.height + 4
                width: listview.width
                Text {
                    id: text
                    height: Math.min(implicitHeight, 50)
                    x: 2
                    y: 2
                    width: parent.width - 4
                    text: modelData
                    elide: Text.ElideRight
                    wrapMode: Text.WrapAnywhere
                    color: CurrentStyle.textColor
                }
                Image {
                    height: 16
                    width: 16
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 2
                    source: "image://icon/edit-paste"
                    opacity: index == root.selectionActive
                    fillMode: Image.PreserveAspectFit
                    sourceSize: Qt.size(32, 32)
                    Behavior on opacity { PropertyAnimation {} }
                }
                Rectangle {
                    x: 2
                    height: 1
                    width: parent.width - 4
                    anchors.top: parent.top
                    color: CurrentStyle.textColor
                    visible: index > 0
                }
            }
        }
    }

    toolTip: Text {
        width: 150
        height: 60
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        color: CurrentStyle.textColor
        text: Clipboard.text || "No selection"
        elide: Text.ElideRight
        wrapMode: Text.Wrap
    }
}
