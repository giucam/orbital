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
import Orbital.MixerService 1.0

PopupElement {
    id: mixer
    width: 50
    height: 150

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30
    minimumWidth: 40
    minimumHeight: 40
    property int horizontal: (mixer.location == 1 || mixer.location == 3)

    buttonContent: Icon {
        id: icon
        anchors.fill: parent
        icon: chooseIcon()

        onClicked: showPopup()

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.MiddleButton

            onClicked: {
                if (mouse.button == Qt.MiddleButton) {
                    Mixer.toggleMuted()
                }
            }
            onWheel: {
                wheel.accepted = true;
                if (!Mixer.muted) {
                    if (wheel.angleDelta.y > 0)
                        Mixer.increaseMaster();
                    else
                        Mixer.decreaseMaster();
                }
            }
        }

        function chooseIcon() {
            var vol = Mixer.master;
            if (Mixer.muted) {
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

    popupWidth: horizontal ? 180 : 50
    popupHeight: horizontal ? 50 : 180
    popupContent: Item {
        anchors.fill: parent

        Slider {
            id: slider
            anchors.top: mixer.location == 2 ? ic.bottom : parent.top
            anchors.right: mixer.location == 1 ? ic.left : parent.right
            anchors.left: mixer.location == 3 ? ic.right : parent.left
            anchors.bottom: mixer.location == 0 || mixer.location == 4 ? ic.top : parent.bottom
            anchors.margins: 3
            orientation: style.horizontal ? Qt.Horizontal : Qt.Vertical
            maximumValue: 100
            stepSize: 1
            value: Mixer.muted ? 0 : Mixer.master;

            // changing the orientation of the slider makes it lose the value and
            // break the binding, so reset it here
            onOrientationChanged: {
                slider.value = Qt.binding(function() { return Mixer.muted ? 0 : Mixer.master })
            }

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    mouse.accepted = true;
                }
                onPositionChanged: {
                    if (!Mixer.muted) {
                        behavior.enabled = false;
                        if (slider.orientation == Qt.Vertical) {
                            var h = slider.height - 10;
                            var y = mouse.y - 5;
                            Mixer.setMaster((1 - y / h) * 100);
                        } else {
                            var h = slider.width - 10;
                            var y = mouse.x - 5;
                            Mixer.setMaster((y / h) * 100);
                        }
                        behavior.enabled = true;
                    }
                }
                onReleased: {
                    if (!Mixer.muted) {
                        if (slider.orientation == Qt.Vertical) {
                            var h = slider.height - 10;
                            var y = mouse.y - 5;
                            Mixer.setMaster((1 - y / h) * 100);
                        } else {
                            var h = slider.width - 10;
                            var y = mouse.x - 5;
                            Mixer.setMaster((y / h) * 100);
                        }
                    }
                }

                onWheel: {
                    wheel.accepted = true;
                    if (!Mixer.muted) {
                        if (wheel.angleDelta.y > 0)
                            Mixer.increaseMaster();
                        else
                            Mixer.decreaseMaster();
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
                when: mixer.location == 0 || mixer.location == 4
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
            onClicked: Mixer.toggleMuted()
        }
    }

    toolTip: Text {
        width: 150
        height: 40
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        color: CurrentStyle.textColor
        text: Mixer.muted ? qsTr("Muted") : qsTr("Volume at %1\%").arg(Mixer.master)
    }
}
