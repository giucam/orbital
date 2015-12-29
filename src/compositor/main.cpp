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

#include <QCoreApplication>
#include <QCommandLineParser>

#include <weston-1/compositor.h>

#include "backend.h"
#include "compositor.h"

int main(int argc, char **argv)
{
    setenv("QT_MESSAGE_PATTERN", "[%{if-debug}D%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif} %{appname}"
                                 " - %{file}:%{line}] == %{message}", 0);

    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Orbital"));
    app.setApplicationVersion(QStringLiteral("0.1"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Orbital compositor"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption socketOption({ QStringLiteral("S"), QStringLiteral("socket") }, QStringLiteral("Socket name"));
    parser.addOption(socketOption);

    QCommandLineOption backendOption({ QStringLiteral("B"), QStringLiteral("backend") }, QStringLiteral("Backend plugin"), QStringLiteral("name"));
    parser.addOption(backendOption);

    parser.process(app);

    Orbital::StringView backendKey;
    QByteArray opt;
    if (parser.isSet(backendOption)) {
        opt = parser.value(backendOption).toUtf8();
        backendKey = opt;
    } else if (getenv("WAYLAND_DISPLAY")) {
        backendKey = "wayland-backend";
    } else if (getenv("DISPLAY")) {
        backendKey = "x11-backend";
    } else {
        backendKey = "drm-backend";
    }

    Orbital::BackendFactory::searchPlugins();
    Orbital::Backend *backend = Orbital::BackendFactory::createBackend(backendKey);
    if (!backend) {
        return 1;
    }

    Orbital::Compositor compositor(backend);
    if (!compositor.init("")) {
        return 1;
    }

    wl_display_run(compositor.display());
    return 0;
}
