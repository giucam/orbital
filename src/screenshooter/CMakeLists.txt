pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Qml Quick)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES main.cpp)

wayland_add_protocol_client(SOURCES ../../protocol/screenshooter.xml screenshooter)
wayland_add_protocol_client(SOURCES ../../protocol/orbital-authorizer.xml authorizer)

qt5_add_resources(RESOURCES resources.qrc)

list(APPEND defines "LIBEXEC_PATH=\"${CMAKE_INSTALL_PREFIX}/libexec\"")
list(APPEND defines "QT_MESSAGELOGCONTEXT")

add_executable(orbital-screenshooter ${SOURCES} ${RESOURCES})
target_link_libraries(orbital-screenshooter wayland-client Qt5::Widgets Qt5::Qml Qt5::Quick)
set_target_properties(orbital-screenshooter PROPERTIES COMPILE_DEFINITIONS "${defines}")
target_include_directories(orbital-screenshooter PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

install(TARGETS orbital-screenshooter DESTINATION bin)

install(FILES imgur DESTINATION libexec)
