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

import QtQuick 2.1
import Orbital 1.0

Element {
    id: element
    Component.onCompleted: {
        element.update();
    }

    property bool alwaysButton: false
    property int minimumWidth: 100
    property int minimumHeight: 100
    property Item popupContent: null
    property Item buttonContent: null
    property int popupWidth: 0
    property int popupHeight: 0

    property Popup __popup: null

    function getPopup() {
        if (!__popup) {
            __popup = popupComponent.createObject(element, { title: element.prettyName });
            update()
        }
        return __popup;
    }

    function update() {
        if (!popupContent) {
            return;
        }

        if (element.alwaysButton || element.width < minimumWidth || element.height < minimumHeight) {
            popupContent.parent = __popup ? __popup.content : null;
            element.contentItem = popupButton;
        } else {
            popupButton.parent = null;
            element.contentItem = popupContent;
        }
    }
    onWidthChanged: update()
    onHeightChanged: update()
    onMinimumWidthChanged: update();
    onMinimumHeightChanged: update()
    onPopupContentChanged: update()
    onButtonContentChanged: {
        if (buttonContent) {
            buttonContent.parent = popupButton
        }
    }

    function showPopup() {
        getPopup().show();
    }

    StyleItem {
        id: popupButton
        anchors.fill: parent
        component: CurrentStyle.popupLauncher

        Binding { target: popupButton.item; property: "popup"; value: __popup }
    }

    Component {
        id: popupComponent
        Popup {
            id: pp
            parentItem: element
            property string title

            content: StyleItem {
                id: style
                width: element.popupWidth
                height: element.popupHeight

                component: CurrentStyle.popup
                Binding { target: style.item; property: "header"; value: element.prettyName }
            }
        }
    }
}
