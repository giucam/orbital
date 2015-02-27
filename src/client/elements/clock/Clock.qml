/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

import QtQuick 2.4
import QtQuick.Controls 1.3
import Orbital 1.0
import Orbital.DateTimeService 1.0

PopupElement {
    id: element

    property int orientation: (location == 0 || location == 2) ? Qt.Horizontal : Qt.Vertical
    Layout.minimumWidth: textMetrics.width + 10
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: time.contentHeight + 10
    Layout.preferredHeight: time.contentHeight + date.contentHeight

    width: orientation == Qt.Horizontal ? Layout.preferredWidth : 100
    height: 20

    TextMetrics {
        id: textMetrics
        font: time.font
        text: "00:00:00"
        property alias tightHeight: textMetrics.tightBoundingRect.height
    }

    onOrientationChanged: {
        if (orientation == Qt.Horizontal) {
            time.wrapMode = Text.NoWrap;
            time.width = Qt.binding(function() { return time.contentWidth});

            date.wrapMode = Text.NoWrap;
            date.width = Qt.binding(function() { return date.contentWidth});
        } else {
            time.wrapMode = Text.WrapAnywhere;
            time.width = Qt.binding(function() { return element.width});

            date.wrapMode = Text.WrapAnywhere;
            date.width = Qt.binding(function() { return element.width});
        }
    }

    buttonContent: MouseArea {
        anchors.fill: parent
        Text {
            id: time
            height: contentHeight
            anchors.horizontalCenter: parent.horizontalCenter
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
            color: CurrentStyle.textColor
            text: DateTime.time
        }
        Text {
            id: date
            y: textMetrics.tightHeight + 3
            height: parent.height - textMetrics.tightHeight
            anchors.horizontalCenter: parent.horizontalCenter
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
            color: CurrentStyle.textColor
            font.pixelSize: 100
            minimumPixelSize: 1
            text: DateTime.date
            fontSizeMode: Text.Fit
        }

        onClicked: showPopup()
    }

    alwaysButton: true
    popupWidth: 300
    popupHeight: 300
    popupContent: Calendar {
        anchors.fill: parent
    }
}
