
find_package(Qt5Core)
find_library(wayland-compositor weston-waylandcompositor /usr/local/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(SOURCES wayland-backend.cpp)

add_library(wayland-backend SHARED ${SOURCES})
qt5_use_modules(wayland-backend Core)
target_link_libraries(wayland-backend weston ${wayland-compositor})
install(TARGETS wayland-backend DESTINATION lib/orbital/compositor/backends)
