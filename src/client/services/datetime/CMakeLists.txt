
find_package(Qt5Core)
find_package(Qt5DBus)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES datetime.cpp)

add_library(datetime SHARED ${SOURCES})
target_link_libraries(datetime Qt5::Core Qt5::Qml)
qt5_use_modules(datetime Core Qml)

set(dest lib/orbital/qml/Orbital/DateTimeService)
install(TARGETS datetime DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
