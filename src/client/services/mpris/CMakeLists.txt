
find_package(Qt5Core)
find_package(Qt5Qml)
find_package(Qt5DBus)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(SOURCES mprisservice.cpp)

add_library(mprisservice SHARED ${SOURCES})

target_link_libraries(mprisservice   Qt5::Core Qt5::Widgets Qt5::Qml Qt5::DBus Qt5::Quick)

set(dest lib/orbital/qml/Orbital/MprisService)
set_target_properties(mprisservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
install(TARGETS mprisservice DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
