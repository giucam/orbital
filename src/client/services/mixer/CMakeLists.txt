
find_package(Qt5Core)
find_package(Qt5Qml)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES mixerservice.cpp)

add_library(mixerservice SHARED ${SOURCES})
qt5_use_modules(mixerservice Core Qml)
install(TARGETS mixerservice DESTINATION lib/orbital/services)
