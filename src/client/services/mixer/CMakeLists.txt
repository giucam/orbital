
find_package(Qt5Core)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES mixerservice.cpp)

add_library(mixerservice SHARED ${SOURCES})
qt5_use_modules(mixerservice Core)
target_link_libraries(mixerservice asound)
install(TARGETS mixerservice DESTINATION lib/orbital/services)
