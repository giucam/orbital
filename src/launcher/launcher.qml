/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

Rectangle {
    id: root
    color: "white"
    signal selected(string exec)

    function reset() {
        text.text = ""
    }

    TextField {
        id: text
        height: parent.height
        width: 100
        focus: true
        placeholderText: "Run program"

        style: TextFieldStyle {
            textColor: "black"
            background: Item { }
        }

        onTextChanged: {
            view.currentIndex = 0;
            matcherModel.expression = text.text;
        }
        onAccepted: {
            root.selected(view.currentItem.text)
        }

        Keys.onDownPressed: {
            if (view.currentIndex > 0) {
                view.currentIndex--;
            }
            event.accepted = true;
        }
        Keys.onUpPressed: {
            if (view.currentIndex < view.count - 2) {
                view.currentIndex++;
            }
            event.accepted = true;
        }
        Keys.onEscapePressed: {
            root.selected("")
            event.accepted = true;
        }
        Keys.onPressed: {
            if (event.key == Qt.Key_Home) {
                view.currentIndex = 0;
                event.accepted = true;
            } else if (event.key == Qt.Key_End) {
                view.currentIndex = view.count - 2;
                event.accepted = true;
            } else if (event.key == Qt.Key_U && event.modifiers == Qt.ControlModifier) {
                text.text = "";
                event.accepted = true;
            }
        }
    }

    ListView {
        id: view
        model: matcherModel
        orientation: ListView.Horizontal
        height: parent.height
        anchors.left: text.right
        anchors.right: parent.right
        spacing: 5
        clip: true
        delegate: Text {
            text: display ? display : ""
            height: root.height
            verticalAlignment: Text.AlignVCenter
        }
        highlightMoveDuration: 200
        highlightMoveVelocity: -1
        highlight: Rectangle {
            color: "lightsteelblue"
            radius: 5
        }
    }

    width: availableWidth
    height: 30
}
