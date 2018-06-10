pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#include_directories(${WaylandClient_INCLUDE_DIRS})

set(SOURCES main.cpp matchermodel.cpp)

wayland_add_protocol_client(SOURCES ../../protocol/desktop-shell.xml desktop-shell)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(orbital-launcher ${SOURCES} ${RESOURCES} ${QM_FILES})
target_link_libraries(orbital-launcher wayland-client Qt5::Qml Qt5::Quick)
set_target_properties(orbital-launcher PROPERTIES COMPILE_DEFINITIONS "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital/launcher\"")
target_include_directories(orbital-launcher PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

install(TARGETS orbital-launcher DESTINATION bin)
