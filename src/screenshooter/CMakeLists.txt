pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${WaylandClient_INCLUDE_DIRS})

foreach(dir ${Qt5Gui_INCLUDE_DIRS})
    include_directories(${dir}/${Qt5Gui_VERSION_STRING}/QtGui/)
endforeach(dir)

set(SOURCES main.cpp)

wayland_add_protocol_client(SOURCES ../../protocol/screenshooter.xml screenshooter)
wayland_add_protocol_client(SOURCES ../../protocol/orbital-authorizer.xml authorizer)

qt5_add_resources(RESOURCES resources.qrc)

list(APPEND defines "LIBEXEC_PATH=\"${CMAKE_INSTALL_PREFIX}/libexec\"")
list(APPEND defines "QT_MESSAGELOGCONTEXT")

add_executable(orbital-screenshooter ${SOURCES} ${RESOURCES})
qt5_use_modules(orbital-screenshooter Widgets Qml Quick)
target_link_libraries(orbital-screenshooter wayland-client)
set_target_properties(orbital-screenshooter PROPERTIES COMPILE_DEFINITIONS "${defines}")

install(TARGETS orbital-screenshooter DESTINATION libexec)

install(FILES imgur DESTINATION libexec)
