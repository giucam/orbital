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
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <compositor-drm.h>

#include "drm-backend.h"

namespace Orbital {

QJsonObject outputs;

DrmBackend::DrmBackend()
{

}

static weston_drm_backend_output_mode configureOutput(weston_compositor *compositor, bool useCurrentMode, const char *name, weston_drm_backend_output_config *config)
{
    weston_drm_backend_output_mode mode = WESTON_DRM_BACKEND_OUTPUT_PREFERRED;
    config->base.scale = 1;
    config->base.transform = WL_OUTPUT_TRANSFORM_NORMAL;
    config->gbm_format = nullptr;
    config->seat = nullptr;
    config->modeline = nullptr;

    if (outputs.contains(QLatin1String(name))) {
        QJsonObject outputObj = outputs[QLatin1String(name)].toObject();
        QString mode = outputObj[QStringLiteral("mode")].toString();
        if (mode == QStringLiteral("off")) {
            mode = WESTON_DRM_BACKEND_OUTPUT_OFF;
        } else if (mode == QStringLiteral("current")) {
            mode = WESTON_DRM_BACKEND_OUTPUT_CURRENT;
        } else if (mode != QStringLiteral("preferred")) {
            config->modeline = strdup(qPrintable(mode));
        }

        int scale = outputObj[QStringLiteral("scale")].toInt();
        if (scale > 0) {
            config->base.scale = scale;
        } else {
            qWarning("Invalid scale %d for output '%s'.", scale, name);
        }
    }

    return mode;
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

    weston_drm_backend_config config;
    config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;
    config.base.struct_size = sizeof(config);
    config.connector = 0;
    config.tty = 0;
    config.use_pixman = false;
    config.seat_id = nullptr;
    config.gbm_format = nullptr;
    config.use_current_mode = false;
    config.configure_output = configureOutput;
    config.configure_device = configureDevice;

    int ret = weston_compositor_load_backend(c, WESTON_BACKEND_DRM, &config.base);
    return ret == 0;
}

}
