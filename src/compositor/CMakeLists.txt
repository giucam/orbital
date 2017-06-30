
pkg_check_modules(WaylandServer wayland-server REQUIRED)
pkg_check_modules(libweston libweston-3 REQUIRED)
pkg_check_modules(libweston-desktop libweston-desktop-3 REQUIRED)

find_package(Qt5Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include_directories(${WaylandServer_INCLUDE_DIRS} /usr/include/pixman-1 ${libweston_INCLUDE_DIRS}
${libweston-desktop_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../utils/)
link_directories(${libweston_LIBRARY_DIRS} ${WaylandServer_LIBRARY_DIRS})

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
    focusscope.cpp
    animation.cpp
    transform.cpp
    binding.cpp
    xwayland.cpp
    pager.cpp
    surface.cpp
    dropdown.cpp
    screenshooter.cpp
    clipboard.cpp
    dashboard.cpp
    gammacontrol.cpp
    authorizer.cpp
    debug.cpp
    ../utils/stringview.cpp
    ../utils/desktopfile.cpp
    effect.cpp
    effects/zoomeffect.cpp
    effects/desktopgrid.cpp
    weston-desktop/wdesktop.cpp
    desktop-shell/desktop-shell.cpp
    desktop-shell/desktop-shell-splash.cpp
    desktop-shell/desktop-shell-window.cpp
    desktop-shell/desktop-shell-notifications.cpp
    desktop-shell/desktop-shell-launcher.cpp
    desktop-shell/desktop-shell-workspace.cpp
    desktop-shell/desktop-shell-settings.cpp
    fmt/format.cc
    fmt/ostream.cc)

wayland_add_protocol_server(SOURCES ../../protocol/desktop-shell.xml desktop-shell)
wayland_add_protocol_server(SOURCES ../../protocol/dropdown.xml dropdown)
wayland_add_protocol_server(SOURCES ../../protocol/screenshooter.xml screenshooter)
wayland_add_protocol_server(SOURCES ../../protocol/orbital-clipboard.xml clipboard)
wayland_add_protocol_server(SOURCES ../../protocol/gamma-control.xml gammacontrol)
wayland_add_protocol_server(SOURCES ../../protocol/orbital-authorizer.xml authorizer)
wayland_add_protocol_server(SOURCES ../../protocol/orbital-authorizer-helper.xml authorizer-helper)

list(APPEND defines "LIBRARIES_PATH=\"${CMAKE_INSTALL_PREFIX}/lib/orbital\"")
list(APPEND defines "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital\"")
list(APPEND defines "LIBEXEC_PATH=\"${CMAKE_INSTALL_PREFIX}/libexec\"")
list(APPEND defines "BIN_PATH=\"${CMAKE_INSTALL_PREFIX}/bin\"")
list(APPEND defines "QT_MESSAGELOGCONTEXT")

add_executable(orbital ${SOURCES})
qt5_use_modules(orbital Core)
target_link_libraries(orbital wayland-server ${libweston_LIBRARIES} ${libweston-desktop_LIBRARIES} pixman-1 xkbcommon)
set_target_properties(orbital PROPERTIES COMPILE_DEFINITIONS "${defines}")

install(TARGETS orbital DESTINATION bin)

add_subdirectory(x11-backend)
add_subdirectory(drm-backend)
add_subdirectory(wayland-backend)

# add_executable(orbital-launch orbital-launch.cpp)
# target_link_libraries(orbital-launch weston-launcher-1)
# set_target_properties(orbital-launch PROPERTIES COMPILE_DEFINITIONS "${defines}")

# install(TARGETS orbital-launch
#         DESTINATION bin
#         PERMISSIONS SETUID OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
