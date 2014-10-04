pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)

set(CMAKE_AUTOMOC ON)

include_directories(${WaylandClient_INCLUDE_DIRS})

foreach(dir ${Qt5Gui_INCLUDE_DIRS})
    include_directories(${dir}/${Qt5Gui_VERSION_STRING}/QtGui/)
endforeach(dir)

set(SOURCES main.cpp)

wayland_add_protocol_client(SOURCES ../../protocol/screenshooter.xml screenshooter)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(orbital-screenshooter ${SOURCES} ${RESOURCES})
qt5_use_modules(orbital-screenshooter Widgets Qml Quick)
target_link_libraries(orbital-screenshooter wayland-client)

install(TARGETS orbital-screenshooter DESTINATION libexec)
