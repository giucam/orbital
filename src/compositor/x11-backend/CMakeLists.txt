
find_package(Qt5Core)
find_library(x11-compositor weston-x11compositor /usr/local/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(SOURCES x11-backend.cpp)

add_library(x11-backend SHARED ${SOURCES})
qt5_use_modules(x11-backend Core)
target_link_libraries(x11-backend weston ${x11-compositor} xcb-shm xcb-xkb)
install(TARGETS x11-backend DESTINATION lib/orbital/compositor/backends)
