

pkg_check_modules(WaylandServer wayland-server REQUIRED)
pkg_check_modules(Pixman pixman-1 REQUIRED)
pkg_check_modules(Weston weston REQUIRED)

include_directories(
    ${WaylandServer_INCLUDE_DIRS}
    ${Pixman_INCLUDE_DIRS}
    ${Weston_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/effects
)

set(SOURCES
    shell.cpp
    layer.cpp
    shellsurface.cpp
    shellseat.cpp
    workspace.cpp
    effect.cpp
    transform.cpp
    animation.cpp
    desktop-shell.cpp
    inputpanel.cpp
    effects/scaleeffect.cpp
    effects/griddesktops.cpp
    effects/zoomeffect.cpp
    effects/fademovingeffect.cpp
    effects/inoutsurfaceeffect.cpp
    effects/minimizeeffect.cpp)

wayland_add_protocol_server(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
    desktop-shell
)

wayland_add_protocol_server(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/input-method.xml
    input-method
)

add_library(orbital-shell SHARED ${SOURCES})
set_target_properties(orbital-shell PROPERTIES PREFIX "")

install(TARGETS orbital-shell DESTINATION lib/orbital)
