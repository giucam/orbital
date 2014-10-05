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

#include <weston/compositor.h>

#include "backend.h"
#include "compositor.h"

int main(int argc, char **argv)
{
    setenv("QT_MESSAGE_PATTERN", "[orbital %{type}] %{message}", 0);

    QCoreApplication app(argc, argv);
    app.setApplicationName("Orbital");
    app.setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Orbital compositor");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption socketOption(QStringList() << "S" << "socket", "Socket name");
    parser.addOption(socketOption);

    QCommandLineOption backendOption(QStringList() << "B" << "backend", "Backend plugin", QLatin1String("name"));
    parser.addOption(backendOption);

    parser.process(app);

    QString backendKey;
    if (parser.isSet(backendOption)) {
        backendKey = parser.value(backendOption);
    } else if (getenv("WAYLAND_DISPLAY")) {
        backendKey = "wayland-backend";
    } else if (getenv("DISPLAY")) {
        backendKey ="x11-backend";
    } else {
        backendKey ="drm-backend";
    }

    Orbital::BackendFactory::searchPlugins();
    Orbital::Backend *backend = Orbital::BackendFactory::createBackend(backendKey);
    if (!backend) {
        return 1;
    }

    Orbital::Compositor compositor(backend);
    if (!compositor.init(QString())) {
        return 1;
    }

    return app.exec();


// 	int ret = EXIT_SUCCESS;
// 	struct wl_display *display;
// 	struct weston_compositor *ec;
// 	struct wl_event_source *signals[4];
// 	struct wl_event_loop *loop;
// 	struct weston_compositor
// 		*(*backend_init)(struct wl_display *display,
// 				int *argc, char *argv[],
// 				struct weston_config *config);
// 	int i, fd;
// 	char *backend = NULL;
// 	char *option_backend = NULL;
// 	char *shell = NULL;
// 	char *option_shell = NULL;
// 	char *modules, *option_modules = NULL;
// 	char *log = NULL;
// 	char *server_socket = NULL, *end;
// 	int32_t idle_time = 300;
// 	int32_t help = 0;
// 	const char *socket_name = NULL;
// 	int32_t version = 0;
// 	int32_t noconfig = 0;
// 	struct weston_config *config = NULL;
// 	struct weston_config_section *section;
// 	struct wl_client *primary_client;
// 	struct wl_listener primary_client_destroyed;
//
// 	const struct weston_option core_options[] = {
// 		{ WESTON_OPTION_STRING, "backend", 'B', &option_backend },
// 		{ WESTON_OPTION_STRING, "shell", 0, &option_shell },
// 		{ WESTON_OPTION_STRING, "socket", 'S', &socket_name },
// 		{ WESTON_OPTION_INTEGER, "idle-time", 'i', &idle_time },
// 		{ WESTON_OPTION_STRING, "modules", 0, &option_modules },
// 		{ WESTON_OPTION_STRING, "log", 0, &log },
// 		{ WESTON_OPTION_BOOLEAN, "help", 'h', &help },
// 		{ WESTON_OPTION_BOOLEAN, "version", 0, &version },
// 		{ WESTON_OPTION_BOOLEAN, "no-config", 0, &noconfig },
// 	};
//
// 	parse_options(core_options, ARRAY_LENGTH(core_options), &argc, argv);
//
// 	if (help)
// 		usage(EXIT_SUCCESS);
//
// 	if (version) {
// 		printf(PACKAGE_STRING "\n");
// 		return EXIT_SUCCESS;
// 	}
//
// 	weston_log_file_open(log);
//
// 	weston_log("%s\n"
// 		STAMP_SPACE "%s\n"
// 		STAMP_SPACE "Bug reports to: %s\n"
// 		STAMP_SPACE "Build: %s\n",
// 		PACKAGE_STRING, PACKAGE_URL, PACKAGE_BUGREPORT,
// 		BUILD_ID);
// 	log_uname();
//
// 	verify_xdg_runtime_dir();
//
// 	display = wl_display_create();
//
// 	loop = wl_display_get_event_loop(display);
// 	signals[0] = wl_event_loop_add_signal(loop, SIGTERM, on_term_signal,
// 					display);
// 	signals[1] = wl_event_loop_add_signal(loop, SIGINT, on_term_signal,
// 					display);
// 	signals[2] = wl_event_loop_add_signal(loop, SIGQUIT, on_term_signal,
// 					display);
//
// 	wl_list_init(&child_process_list);
// 	signals[3] = wl_event_loop_add_signal(loop, SIGCHLD, sigchld_handler,
// 					NULL);
//
// 	if (!signals[0] || !signals[1] || !signals[2] || !signals[3]) {
// 		ret = EXIT_FAILURE;
// 		goto out_signals;
// 	}
//
// 	if (noconfig == 0)
// 		config = weston_config_parse("weston.ini");
// 	if (config != NULL) {
// 		weston_log("Using config file '%s'\n",
// 			weston_config_get_full_path(config));
// 	} else {
// 		weston_log("Starting with no config file.\n");
// 	}
// 	section = weston_config_get_section(config, "core", NULL, NULL);
//
// 	if (option_backend)
// 		backend = strdup(option_backend);
// 	else
// 		weston_config_section_get_string(section, "backend", &backend,
// 						NULL);
//
// 	if (!backend) {
// 		if (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET"))
// 			backend = strdup("wayland-backend.so");
// 		else if (getenv("DISPLAY"))
// 			backend = strdup("x11-backend.so");
// 		else
// 			backend = strdup(WESTON_NATIVE_BACKEND);
// 	}
//
// 	backend_init = weston_load_module(backend, "backend_init");
// 	free(backend);
// 	if (!backend_init) {
// 		ret = EXIT_FAILURE;
// 		goto out_signals;
// 	}
//
// 	ec = backend_init(display, &argc, argv, config);
// 	if (ec == NULL) {
// 		weston_log("fatal: failed to create compositor\n");
// 		ret = EXIT_FAILURE;
// 		goto out_signals;
// 	}
//
// 	catch_signals();
// 	segv_compositor = ec;
//
// 	ec->idle_time = idle_time;
// 	ec->default_pointer_grab = NULL;
//
// 	if (init_config(ec, config) < 0 || ec->setup(ec) < 0) {
// 		ret = EXIT_FAILURE;
// 		goto out;
// 	}
//
// 	for (i = 1; i < argc; i++)
// 		weston_log("fatal: unhandled option: %s\n", argv[i]);
// 	if (argc > 1) {
// 		ret = EXIT_FAILURE;
// 		goto out;
// 	}
//
// 	server_socket = getenv("WAYLAND_SERVER_SOCKET");
// 	if (server_socket) {
// 		weston_log("Running with single client\n");
// 		fd = strtol(server_socket, &end, 0);
// 		if (*end != '\0')
// 			fd = -1;
// 	} else {
// 		fd = -1;
// 	}
//
// 	if (fd != -1) {
// 		primary_client = wl_client_create(display, fd);
// 		if (!primary_client) {
// 			weston_log("fatal: failed to add client: %m\n");
// 			ret = EXIT_FAILURE;
// 			goto out;
// 		}
// 		primary_client_destroyed.notify =
// 			handle_primary_client_destroyed;
// 		wl_client_add_destroy_listener(primary_client,
// 					&primary_client_destroyed);
// 	} else {
// 		if (socket_name) {
// 			if (wl_display_add_socket(display, socket_name)) {
// 				weston_log("fatal: failed to add socket: %m\n");
// 				ret = EXIT_FAILURE;
// 				goto out;
// 			}
// 		} else {
// 			socket_name = wl_display_add_socket_auto(display);
// 			if (!socket_name) {
// 				weston_log("fatal: failed to add socket: %m\n");
// 				ret = EXIT_FAILURE;
// 				goto out;
// 			}
// 		}
//
// 		setenv("WAYLAND_DISPLAY", socket_name, 1);
// 	}
//
// 	if (option_shell)
// 		shell = strdup(option_shell);
// 	else
// 		weston_config_section_get_string(section, "shell", &shell,
// 						"desktop-shell.so");
//
// 	if (load_modules(ec, shell, &argc, argv, config) < 0) {
// 		free(shell);
// 		goto out;
// 	}
// 	free(shell);
//
// 	weston_config_section_get_string(section, "modules", &modules, "");
// 	if (load_modules(ec, modules, &argc, argv, config) < 0) {
// 		free(modules);
// 		goto out;
// 	}
// 	free(modules);
//
// 	if (load_modules(ec, option_modules, &argc, argv, config) < 0)
// 		goto out;
//
// 	weston_compositor_wake(ec);
//
// 	wl_display_run(display);
//
// out:
// 	if (ec)
// 		weston_compositor_destroy(ec);
//
// out_signals:
// 	for (i = ARRAY_LENGTH(signals) - 1; i >= 0; i--)
// 		if (signals[i])
// 			wl_event_source_remove(signals[i]);
//
// 	wl_display_destroy(display);
//
// 	weston_log_file_close();
//
// 	return ret;
}
