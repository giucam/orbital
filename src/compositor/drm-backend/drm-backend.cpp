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

DrmBackend::DrmBackend()
{

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
    auto outputs = doc.object()[QStringLiteral("Compositor")].toObject()[QStringLiteral("Outputs")].toObject();

    weston_drm_backend_config config;
    config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;
    config.base.struct_size = sizeof(config);
    config.connector = 0;
    config.tty = 0;
    config.use_pixman = false;
    config.seat_id = nullptr;
    config.gbm_format = nullptr;
    config.configure_device = configureDevice;

    if (weston_compositor_load_backend(c, WESTON_BACKEND_DRM, &config.base) != 0) {
        return false;
    }

    const struct weston_drm_output_api *api = weston_drm_output_get_api(c);
    if (!api) {
        qWarning("Cannot use weston_drm_output_api.");
        return false;
    }

    m_pendingListener.setNotify([api, outputs](Listener *, void *data) {
        auto output = static_cast<weston_output *>(data);

        int scale = 1;
        weston_drm_backend_output_mode mode = WESTON_DRM_BACKEND_OUTPUT_PREFERRED;
        QString modeline;

        QJsonValue configValue = outputs[QLatin1String(output->name)];
        if (!configValue.isUndefined()) {
            QJsonObject config = configValue.toObject();

            QString modeString = config[QStringLiteral("mode")].toString();
            if (modeString == QStringLiteral("off")) {
                weston_output_disable(output);
                return;
            } else if (modeString == QStringLiteral("current")) {
                mode = WESTON_DRM_BACKEND_OUTPUT_CURRENT;
            } else if (modeString != QStringLiteral("preferred")) {
                modeline = modeString;
            }

            auto scalevalue = config[QStringLiteral("scale")];
            if (scalevalue.isDouble()) {
                scale = scalevalue.toInt();
            }
        }

        if (api->set_mode(output, mode, qPrintable(modeline)) < 0) {
            qWarning("Failed to configure DRM output '%s'", output->name);
            return;
        }

        weston_output_set_scale(output, scale);
        weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);

        api->set_gbm_format(output, nullptr);
        api->set_seat(output, "");

        weston_output_enable(output);
    });
    m_pendingListener.connect(&c->output_pending_signal);

    return true;
}

}
