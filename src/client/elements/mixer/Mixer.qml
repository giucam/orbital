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
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.0
import Orbital 1.0
import QtQuick.Window 2.1
import QtQuick.Controls 1.0

Element {
    id: mixer
    width: 30
    height: 30

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30

    property variant service: Client.service("MixerService")

    contentItem: StyleItem {
        id: launcher
        anchors.fill: parent
        component: CurrentStyle.popupLauncher

        Binding { target: launcher.item; property: "popup"; value: popup }

        Icon {
            id: icon
            anchors.fill: parent
            icon: chooseIcon()

            onClicked: popup.show()

            MouseArea {
                anchors.fill: parent

                onPressed: mouse.accepted = false
                onWheel: {
                    wheel.accepted = true;
                    if (!service.muted) {
                        if (wheel.angleDelta.y > 0)
                            service.increaseMaster();
                        else
                            service.decreaseMaster();
                    }
                }
            }

            function chooseIcon() {
                var vol = service.master;
                if (service.muted) {
                    return "image://icon/audio-volume-muted";
                } else if (vol > 66) {
                    return "image://icon/audio-volume-high";
                } else if (vol > 33) {
                    return "image://icon/audio-volume-medium";
                } else {
                    return "image://icon/audio-volume-low";
                }
            }
        }
    }

    Popup {
        id: popup
        parentItem: mixer

        content: StyleItem {
            id: style
            width: 50
            height: 180

            component: CurrentStyle.popup

            Binding { target: style.item; property: "header"; value: "Mixer" }

            Slider {
                id: slider
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.bottom: ic.top
                anchors.margins: 3
                orientation: Qt.Vertical
                maximumValue: 100
                stepSize: 1
                value: service.muted ? 0 : service.master;

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        mouse.accepted = true;
                    }
                    onPositionChanged: {
                        if (!service.muted) {
                            behavior.enabled = false;
                            var h = slider.height - 10;
                            var y = mouse.y - 5;
                            service.setMaster((1 - y / h) * 100);
                            behavior.enabled = true;
                        }
                    }
                    onReleased: {
                        if (!service.muted) {
                            var h = slider.height - 10;
                            var y = mouse.y - 5;
                            service.setMaster((1 - y / h) * 100);
                        }
                    }

                    onWheel: {
                        wheel.accepted = true;
                        if (!service.muted) {
                            if (wheel.angleDelta.y > 0)
                                service.increaseMaster();
                            else
                                service.decreaseMaster();
                        }
                    }
                }

                Behavior on value {
                    id: behavior
                    PropertyAnimation { duration: 200 }
                }
            }
            Icon {
                id: ic
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 3
                width: 20
                height: 20
                icon: icon.icon

                onClicked: service.toggleMuted()
            }
        }
    }

    toolTip: Text {
        width: 150
        height: 40
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        color: CurrentStyle.textColor
        text: service.muted ? qsTr("Muted") : qsTr("Volume at %1\%").arg(service.master)
    }
}
