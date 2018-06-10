
find_package(Qt5Core)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES processlauncher.cpp)

add_library(processlauncher SHARED ${SOURCES})
target_link_libraries(processlauncher Qt5::Core Qt5::Qml)
set(dest lib/orbital/qml/Orbital/ProcessLauncherService)
install(TARGETS processlauncher DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
