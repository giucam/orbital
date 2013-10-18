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
            property int horizontal: (mixer.location == 1 || mixer.location == 3)

            width: horizontal ? 180 : 50
            height: horizontal ? 50 : 180

            component: CurrentStyle.popup

            Binding { target: style.item; property: "header"; value: "Mixer" }

            Slider {
                id: slider
                anchors.top: mixer.location == 2 ? ic.bottom : parent.top
                anchors.right: mixer.location == 1 ? ic.left : parent.right
                anchors.left: mixer.location == 3 ? ic.right : parent.left
                anchors.bottom: mixer.location == 0 ? ic.top : parent.bottom
                anchors.margins: 3
                orientation: style.horizontal ? Qt.Horizontal : Qt.Vertical
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
                            if (slider.orientation == Qt.Vertical) {
                                var h = slider.height - 10;
                                var y = mouse.y - 5;
                                service.setMaster((1 - y / h) * 100);
                            } else {
                                var h = slider.width - 10;
                                var y = mouse.x - 5;
                                service.setMaster((y / h) * 100);
                            }
                            behavior.enabled = true;
                        }
                    }
                    onReleased: {
                        if (!service.muted) {
                            if (slider.orientation == Qt.Vertical) {
                                var h = slider.height - 10;
                                var y = mouse.y - 5;
                                service.setMaster((1 - y / h) * 100);
                            } else {
                                var h = slider.width - 10;
                                var y = mouse.x - 5;
                                service.setMaster((y / h) * 100);
                            }
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
                anchors.margins: 3
                width: 20
                height: 20
                icon: icon.icon

                states: [
                    State {
                        name: "top"
                        when: mixer.location == 0
                        AnchorChanges {
                            target: ic
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    },
                    State {
                        name: "left"
                        when: mixer.location == 1
                        AnchorChanges {
                            target: ic
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    },
                    State {
                        name: "bottom"
                        when: mixer.location == 2
                        AnchorChanges {
                            target: ic
                            anchors.top: parent.top
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    },
                    State {
                        name: "right"
                        when: mixer.location == 3
                        AnchorChanges {
                            target: ic
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                ]
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
