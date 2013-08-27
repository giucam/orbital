
pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)

set(CMAKE_AUTOMOC ON)

include_directories(${WaylandClient_INCLUDE_DIRS})

foreach(dir ${Qt5Widgets_INCLUDE_DIRS})
    include_directories(${dir}/${Qt5Gui_VERSION_STRING}/QtGui/)
endforeach(dir)

set(SOURCES
    main.cpp
    client.cpp
    iconimageprovider.cpp
    shellui.cpp
    uiscreen.cpp
    window.cpp
    filebrowser.cpp
    element.cpp
    grab.cpp
    layout.cpp
    workspace.cpp
    service.cpp
    style.cpp
    styleitem.cpp
    tooltip.cpp
    popup.cpp)

wayland_add_protocol_client(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
    desktop-shell
)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(orbital-client ${SOURCES} ${RESOURCES})
qt5_use_modules(orbital-client Widgets Qml Quick)
target_link_libraries(orbital-client wayland-client)
set_target_properties(orbital-client PROPERTIES COMPILE_DEFINITIONS LIBRARIES_PATH="${CMAKE_INSTALL_PREFIX}/lib/orbital")

install(TARGETS orbital-client DESTINATION libexec)
install(DIRECTORY elements DESTINATION share/orbital)
install(DIRECTORY styles DESTINATION share/orbital)

add_subdirectory(services/mixer)
add_subdirectory(services/login)
add_subdirectory(services/processlauncher)
