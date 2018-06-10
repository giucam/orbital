
find_package(Qt5Core)
find_package(Qt5DBus)
find_package(Qt5Qml)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES statusnotifierservice.cpp
            statusnotifierwatcher.cpp
            statusnotifieritem.cpp
            statusnotifiericonprovider.cpp)

add_library(statusnotifierservice SHARED ${SOURCES})
target_link_libraries(statusnotifierservice Qt5::Core Qt5::Widgets Qt5::DBus Qt5::Gui Qt5::Qml Qt5::Quick)
set_target_properties(statusnotifierservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
set(dest lib/orbital/qml/Orbital/StatusNotifierService)
install(TARGETS statusnotifierservice DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
