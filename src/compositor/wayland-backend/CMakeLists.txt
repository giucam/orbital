
find_package(Qt5Core)
pkg_check_modules(Weston weston REQUIRED)
find_library(wwl wayland-backend.so ${Weston_LIBDIR}/weston-1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(SOURCES wayland-backend.cpp)

add_library(wayland-backend SHARED ${SOURCES})
qt5_use_modules(wayland-backend Core)
target_link_libraries(wayland-backend weston ${wwl})
set_target_properties(wayland-backend PROPERTIES INSTALL_RPATH ${Weston_LIBDIR}/weston-1)
install(TARGETS wayland-backend DESTINATION lib/orbital/compositor/backends)
