
find_package(Qt5Core)
pkg_check_modules(Weston weston REQUIRED)
find_library(wdrm drm-backend.so ${Weston_LIBDIR}/weston)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. /usr/include/libdrm)

set(SOURCES drm-backend.cpp)

add_library(drm-backend SHARED ${SOURCES})
qt5_use_modules(drm-backend Core)
target_link_libraries(drm-backend weston ${wdrm})
set_target_properties(drm-backend PROPERTIES INSTALL_RPATH ${Weston_LIBDIR}/weston)
install(TARGETS drm-backend DESTINATION lib/orbital/compositor/backends)
