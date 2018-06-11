pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES main.cpp)

wayland_add_protocol_client(SOURCES
    ../../protocol/desktop-shell.xml
    desktop-shell
)

file(GLOB translations translations/*.ts)
qt5_add_translation(QM_FILES ${translations})
qt5_add_resources(RESOURCES resources.qrc)

add_executable(orbital-splash ${SOURCES} ${RESOURCES} ${QM_FILES})
target_link_libraries(orbital-splash wayland-client ${WaylandClient_LIBRARIES} Qt5::Qml Qt5::Quick)
set_target_properties(orbital-splash PROPERTIES COMPILE_DEFINITIONS "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital/splash\"")
target_include_directories(orbital-splash PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

install(FILES ${QM_FILES} DESTINATION share/orbital/splash/translations)
install(TARGETS orbital-splash DESTINATION libexec)
