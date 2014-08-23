
find_package(Qt5Core)
find_library(drm-compositor weston-drmcompositor /usr/local/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. /usr/include/libdrm)

set(SOURCES drm-backend.cpp)

add_library(drm-backend SHARED ${SOURCES})
qt5_use_modules(drm-backend Core)
target_link_libraries(drm-backend weston ${drm-compositor} weston-drmcompositor)
install(TARGETS drm-backend DESTINATION lib/orbital/compositor/backends)
