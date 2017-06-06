pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(Qt5LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${WaylandClient_INCLUDE_DIRS})
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

set(SOURCES main.cpp)

wayland_add_protocol_client(SOURCES
    ../../protocol/desktop-shell.xml
    desktop-shell
)

file(GLOB translations translations/*.ts)
qt5_add_translation(QM_FILES ${translations})
qt5_add_resources(RESOURCES resources.qrc)

add_executable(orbital-splash ${SOURCES} ${RESOURCES} ${QM_FILES})
qt5_use_modules(orbital-splash Qml Quick)
target_link_libraries(orbital-splash wayland-client)
set_target_properties(orbital-splash PROPERTIES COMPILE_DEFINITIONS "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital/splash\"")

install(FILES ${QM_FILES} DESTINATION share/orbital/splash/translations)
install(TARGETS orbital-splash DESTINATION libexec)
