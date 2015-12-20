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

#include <unistd.h>
#include <gbm.h>

#include <QDebug>
#include <QHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <weston-1/compositor-drm.h>

#include "drm-backend.h"

namespace Orbital {

QJsonObject outputs;

DrmBackend::DrmBackend()
{

}

static bool parseModeline(const QString &s, drmModeModeInfo *mode)
{
    char hsync[16];
    char vsync[16];
    float fclock;

    mode->type = DRM_MODE_TYPE_USERDEF;
    mode->hskew = 0;
    mode->vscan = 0;
    mode->vrefresh = 0;
    mode->flags = 0;

    if (sscanf(qPrintable(s), "%f %hd %hd %hd %hd %hd %hd %hd %hd %15s %15s",
           &fclock,
           &mode->hdisplay,
           &mode->hsync_start,
           &mode->hsync_end,
           &mode->htotal,
           &mode->vdisplay,
           &mode->vsync_start,
           &mode->vsync_end,
           &mode->vtotal, hsync, vsync) != 11)
        return false;

    mode->clock = fclock * 1000;
    if (strcmp(hsync, "+hsync") == 0)
        mode->flags |= DRM_MODE_FLAG_PHSYNC;
    else if (strcmp(hsync, "-hsync") == 0)
        mode->flags |= DRM_MODE_FLAG_NHSYNC;
    else
        return false;

    if (strcmp(vsync, "+vsync") == 0)
        mode->flags |= DRM_MODE_FLAG_PVSYNC;
    else if (strcmp(vsync, "-vsync") == 0)
        mode->flags |= DRM_MODE_FLAG_NVSYNC;
    else
        return false;

    return true;
}

static void output_data(const char *name, struct drm_output_parameters *data)
{
    if (outputs.contains(QLatin1String(name))) {
        QString mode = outputs[QLatin1String(name)].toObject()[QStringLiteral("mode")].toString();
        if (mode == QStringLiteral("off")) {
            data->mode.config = DRM_OUTPUT_CONFIG_OFF;
        } else if (mode == QStringLiteral("preferred")) {
            data->mode.config = DRM_OUTPUT_CONFIG_PREFERRED;
        } else if (mode == QStringLiteral("current")) {
            data->mode.config = DRM_OUTPUT_CONFIG_CURRENT;
        } else if (sscanf(qPrintable(mode), "%dx%d", &data->mode.width, &data->mode.height) == 2) {
            data->mode.config = DRM_OUTPUT_CONFIG_MODE;
        } else if (parseModeline(mode, &data->mode.modeline)) {
            data->mode.config = DRM_OUTPUT_CONFIG_MODELINE;
        } else {
            qWarning("Invalid mode '%s' for output '%s'.", qPrintable(mode), name);
            data->mode.config = DRM_OUTPUT_CONFIG_PREFERRED;
        }
    } else {
        data->mode.config = DRM_OUTPUT_CONFIG_PREFERRED;
    }

    data->scale = 1;
    data->transform = WL_OUTPUT_TRANSFORM_NORMAL;
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
    struct drm_backend_parameters param;
    memset(&param, 0, sizeof param);

    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString configFile = path + QLatin1String("/orbital/orbital.conf");

    QFile file(configFile);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        file.close();
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    outputs = doc.object()[QStringLiteral("Compositor")].toObject()[QStringLiteral("Outputs")].toObject();

    param.tty = 0;
    param.format = GBM_FORMAT_XRGB8888;
    param.get_output_parameters = output_data;
    param.configure_device = configureDevice;
    param.seat_id = "seat0";
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

    drm_backend *b = drm_backend_create(c, &param);
//     free(format);

    return b;
}

}
