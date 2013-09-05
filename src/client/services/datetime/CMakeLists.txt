
find_package(Qt5Core)
find_package(Qt5DBus)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES datetime.cpp)

add_library(datetime SHARED ${SOURCES})
qt5_use_modules(datetime Core)
install(TARGETS datetime DESTINATION lib/orbital/services)
