
pkg_check_modules(WaylandServer wayland-server REQUIRED)
pkg_check_modules(Weston weston REQUIRED)

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
    animation.cpp
    transform.cpp
    binding.cpp
    xwayland.cpp
    pager.cpp
    surface.cpp
    dropdown.cpp
    screenshooter.cpp
    effect.cpp
    effects/zoomeffect.cpp
    wlshell/wlshell.cpp
    wlshell/wlshellsurface.cpp
    desktop-shell/desktop-shell.cpp
    desktop-shell/desktop-shell-splash.cpp
    desktop-shell/desktop-shell-window.cpp
    desktop-shell/desktop-shell-notifications.cpp
    desktop-shell/desktop-shell-workspace.cpp)

wayland_add_protocol_server(SOURCES ../../protocol/desktop-shell.xml desktop-shell)
wayland_add_protocol_server(SOURCES ../../protocol/dropdown.xml dropdown)
wayland_add_protocol_server(SOURCES ../../protocol/screenshooter.xml screenshooter)

list(APPEND defines "LIBRARIES_PATH=\"${CMAKE_INSTALL_PREFIX}/lib/orbital\"")
list(APPEND defines "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital\"")
list(APPEND defines "LIBEXEC_PATH=\"${CMAKE_INSTALL_PREFIX}/libexec\"")
list(APPEND defines "BIN_PATH=\"${CMAKE_INSTALL_PREFIX}/bin\"")

add_executable(orbital ${SOURCES})
qt5_use_modules(orbital Core)
target_link_libraries(orbital wayland-server weston weston-xwayland pixman-1)
set_target_properties(orbital PROPERTIES COMPILE_DEFINITIONS "${defines}")

install(TARGETS orbital DESTINATION bin)

add_subdirectory(x11-backend)
add_subdirectory(drm-backend)
add_subdirectory(wayland-backend)

add_executable(orbital-launch orbital-launch.cpp)
target_link_libraries(orbital-launch weston-launcher)
set_target_properties(orbital-launch PROPERTIES COMPILE_DEFINITIONS "${defines}")

install(TARGETS orbital-launch
        DESTINATION bin
        PERMISSIONS SETUID OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
