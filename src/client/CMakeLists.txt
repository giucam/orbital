
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)

set(CMAKE_AUTOMOC ON)

include_directories(/opt/qt5/include/QtGui/5.1.0/QtGui/)

set(SOURCES
    main.cpp
    client.cpp
    processlauncher.cpp
    volumecontrol.cpp
    shellitem.cpp
    iconimageprovider.cpp
    shellui.cpp)

wayland_add_protocol_client(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
    desktop-shell
)

add_executable(orbital-client ${SOURCES})
qt5_use_modules(orbital-client Widgets Qml Quick)
target_link_libraries(orbital-client wayland-client asound)
