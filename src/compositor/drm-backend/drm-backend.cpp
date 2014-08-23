
#include <weston/compositor-drm.h>

#include "drm-backend.h"

namespace Orbital {

DrmBackend::DrmBackend()
{

}

static struct drm_backend_output_data *request_output_data(struct drm_backend *b, const char *name)
{
    struct drm_backend_output_data *data;
    char *s;

    data = (drm_backend_output_data *)zalloc(sizeof *data);
    if (data == NULL)
        return NULL;

    data->mode.config = DRM_OUTPUT_CONFIG_PREFERRED;
    data->scale = 1;
    data->transform = WL_OUTPUT_TRANSFORM_NORMAL;
    data->format = NULL;
    data->seat = NULL;

    return data;
}

static void configureDevice(struct weston_compositor *compositor, struct libinput_device *device)
{
//     struct weston_config_section *s;
//     int enable_tap;
//     int enable_tap_default;
//
//     s = weston_config_get_section(backend_config,
//                     "libinput", NULL, NULL);
//
//     if (libinput_device_config_tap_get_finger_count(device) > 0) {
//         enable_tap_default =
//             libinput_device_config_tap_get_default_enabled(
//                 device);
//         weston_config_section_get_bool(s, "enable_tap",
//                         &enable_tap,
//                         enable_tap_default);
//         libinput_device_config_tap_set_enabled(device,
//                             enable_tap);
//     }
}

bool DrmBackend::init(weston_compositor *c)
{
    struct weston_config_section *section;
    struct drm_backend_parameters param = { 0, };

    param.tty = 1;
    param.format = NULL;
//     char *format;

//     backend_config = config;
//
//     const struct weston_option drm_options[] = {
//         { WESTON_OPTION_INTEGER, "connector", 0, &param.connector },
//         { WESTON_OPTION_STRING, "seat", 0, &param.seat_id },
//         { WESTON_OPTION_INTEGER, "tty", 0, &param.tty },
//         { WESTON_OPTION_BOOLEAN, "current-mode", 0, &option_current_mode },
//         { WESTON_OPTION_BOOLEAN, "use-pixman", 0, &param.use_pixman },
//     };
//
//     section = weston_config_get_section(config, "core", NULL, NULL);
//     weston_config_section_get_string(section,
//                      "gbm-format", &format, NULL);
//
//
//     param.seat_id = NULL;
//
//     parse_options(drm_options, ARRAY_LENGTH(drm_options), argc, argv);

    drm_backend *b = drm_backend_create(c, &param,
                           request_output_data,
                           configureDevice);
//     free(format);

    return b;
}

}
