
find_package(Qt5Core)
find_package(Qt5DBus)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES loginservice.cpp)

add_library(loginservice SHARED ${SOURCES})
qt5_use_modules(loginservice Core DBus)
install(TARGETS loginservice DESTINATION lib/orbital/services)
