
pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(Qt5DBus)

set(CMAKE_AUTOMOC ON)

include_directories(${WaylandClient_INCLUDE_DIRS})

foreach(dir ${Qt5Widgets_INCLUDE_DIRS})
    include_directories(${dir}/${Qt5Gui_VERSION_STRING}/QtGui/)
endforeach(dir)

set(SOURCES
    main.cpp
    client.cpp
    processlauncher.cpp
    volumecontrol.cpp
    iconimageprovider.cpp
    shellui.cpp
    uiscreen.cpp
    window.cpp
    filebrowser.cpp
    element.cpp
    grab.cpp
    layout.cpp
    workspace.cpp)

wayland_add_protocol_client(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
    desktop-shell
)

add_executable(orbital-client ${SOURCES})
qt5_use_modules(orbital-client Widgets Qml Quick DBus)
target_link_libraries(orbital-client wayland-client asound)
