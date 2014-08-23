
pkg_check_modules(WaylandServer wayland-server REQUIRED)

find_package(Qt5Core)

set(CMAKE_AUTOMOC ON)

include_directories(${WaylandServer_INCLUDE_DIRS} /usr/include/pixman-1)

set(SOURCES
    main.cpp
    backend.cpp
    compositor.cpp
    shell.cpp
    shellsurface.cpp
    shellview.cpp
    interface.cpp
    view.cpp
    layer.cpp
    workspace.cpp
    output.cpp
    dummysurface.cpp
    seat.cpp
    wlshell/wlshell.cpp
    wlshell/wlshellsurface.cpp)

# wayland_add_protocol_server(SOURCES
#     ${Nuclear_PREFIX}/share/nuclear-shell/nuclear-desktop-shell.xml
#     desktop-shell
# )
# wayland_add_protocol_client(SOURCES
#     ${Nuclear_PREFIX}/share/nuclear-shell/nuclear-settings.xml
#     settings
# )

list(APPEND defines "LIBRARIES_PATH=\"${CMAKE_INSTALL_PREFIX}/lib/orbital\"")
list(APPEND defines "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital\"")
list(APPEND defines "LIBEXEC_PATH=\"${CMAKE_INSTALL_PREFIX}/libexec\"")

add_executable(orbital ${SOURCES})
qt5_use_modules(orbital Core)
target_link_libraries(orbital wayland-server weston pixman-1)
set_target_properties(orbital PROPERTIES COMPILE_DEFINITIONS "${defines}")

install(TARGETS orbital DESTINATION bin)

add_subdirectory(x11-backend)
add_subdirectory(drm-backend)
