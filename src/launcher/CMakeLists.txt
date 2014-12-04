pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(Qt5LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${WaylandClient_INCLUDE_DIRS})

foreach(dir ${Qt5Gui_INCLUDE_DIRS})
    include_directories(${dir}/${Qt5Gui_VERSION_STRING}/QtGui/)
endforeach(dir)

set(SOURCES main.cpp matchermodel.cpp)

wayland_add_protocol_client(SOURCES ../../protocol/desktop-shell.xml desktop-shell)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(orbital-launcher ${SOURCES} ${RESOURCES} ${QM_FILES})
qt5_use_modules(orbital-launcher Qml Quick)
target_link_libraries(orbital-launcher wayland-client)
set_target_properties(orbital-launcher PROPERTIES COMPILE_DEFINITIONS "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital/launcher\"")

install(TARGETS orbital-launcher DESTINATION bin)
