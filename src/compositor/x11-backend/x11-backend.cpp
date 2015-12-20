/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#include <weston-1/compositor-x11.h>

#include "x11-backend.h"

namespace Orbital {

X11Backend::X11Backend()
{

}

bool X11Backend::init(weston_compositor *c)
{
//     struct x11_compositor *compositor;
//     struct weston_compositor *base;
//     struct weston_output *output;
//     struct weston_config_section *section = NULL;
//     const char *section_name;
//     int i, x = 0, output_count = 0;
//     int width, height, scale, count;
//     char *name, *t, *mode;
//     uint32_t transform;
//     int fullscreen = 0;
//     int no_input = 0;
//     int use_pixman = 0;
//
//     const struct weston_option x11_options[] = {
//         { WESTON_OPTION_INTEGER, "width", 0, &option_width },
//         { WESTON_OPTION_INTEGER, "height", 0, &option_height },
//         { WESTON_OPTION_INTEGER, "scale", 0, &option_scale },
//         { WESTON_OPTION_BOOLEAN, "fullscreen", 'f', &fullscreen },
//         { WESTON_OPTION_INTEGER, "output-count", 0, &option_count },
//         { WESTON_OPTION_BOOLEAN, "no-input", 0, &no_input },
//         { WESTON_OPTION_BOOLEAN, "use-pixman", 0, &use_pixman },
//     };
//
//     parse_options(x11_options, ARRAY_LENGTH(x11_options), argc, argv);
//
    int fullscreen = 0;
    int no_input = 0;
    int use_pixman = 0;

    x11_backend *b = x11_backend_create(c,
                     fullscreen,
                     no_input,
                     use_pixman);

    x11_backend_create_output(b, 0, 0, 500,500,
                              fullscreen, no_input, "X1",
                              WL_OUTPUT_TRANSFORM_NORMAL, 1);
    x11_backend_create_output(b, 500, 0, 600,500,
                            fullscreen, no_input, "X2",
                            WL_OUTPUT_TRANSFORM_NORMAL, 1);

    return b;
}

}
