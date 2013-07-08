
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)

set(SOURCES
    main.cpp)

# wayland_add_protocol_server(SOURCES
#     ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
#     desktop-shell
# )

add_executable(orbital-client ${SOURCES})
qt5_use_modules(orbital-client Widgets)
