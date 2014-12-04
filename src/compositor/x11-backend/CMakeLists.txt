
find_package(Qt5Core)
pkg_check_modules(Weston weston REQUIRED)
find_library(wx11 x11-backend.so ${Weston_LIBDIR}/weston-1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(SOURCES x11-backend.cpp)

add_library(x11-backend SHARED ${SOURCES})
qt5_use_modules(x11-backend Core)
target_link_libraries(x11-backend weston-1 ${wx11} xcb-shm xcb-xkb)
set_target_properties(x11-backend PROPERTIES INSTALL_RPATH ${Weston_LIBDIR}/weston-1)
install(TARGETS x11-backend DESTINATION lib/orbital/compositor/backends)
