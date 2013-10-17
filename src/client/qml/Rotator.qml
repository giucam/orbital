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

Item {
    id: root

    property alias angle: rotator.rotation
    property bool outside: false

    default property alias __proxy: rotator.data

    Item {
        id: rotator

        property var rect: outside ? outsideRect(root.width, root.height, rotator.rotation) : insideRect(root.width, root.height, rotator.rotation)
        height: rect.h
        width: rect.w

        y: (root.height - height) / 2
        x: (root.width - width) / 2

        function insideRect(w, h, angle) {
            // This algorithm comes from the first answer on
            // http://stackoverflow.com/questions/16702966/rotate-image-and-crop-out-black-borders
            // Many thanks to the author, "coproc"

            var width_is_longer = w >= h
            var side_long = width_is_longer ? w : h
            var side_short = width_is_longer ? h : w

            var rad = Math.PI * angle / 180
            var sin_a = Math.abs(Math.sin(rad));
            var cos_a = Math.abs(Math.cos(rad));
            var wr, hr;
            if (side_short <= 2. * sin_a * cos_a * side_long) {
                var x = 0.5 * side_short;
                wr = width_is_longer ? x / sin_a : x / cos_a;
                hr = width_is_longer ? x / cos_a : x / sin_a;
            } else {
                var cos_2a = cos_a * cos_a - sin_a * sin_a;
                wr = (w * cos_a - h * sin_a) / cos_2a;
                hr = (h * cos_a - w * sin_a) / cos_2a;
            }

            return { w: wr, h: hr }
        }

        function outsideRect(w, h, angle) {
            var rad = Math.PI * angle / 180
            var s = Math.abs(Math.sin(rad));
            var c = Math.abs(Math.cos(rad));

            var dx = w * c + h * s
            var dy = h * c + w * s
            return { w: Math.abs(dx), h: Math.abs(dy) };
        }
    }
}
