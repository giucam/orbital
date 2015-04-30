
find_program(LRELEASE_EXECUTABLE NAMES lrelease)
function(INSTALL_ELEMENT _sources _dir)
    install(DIRECTORY ${_dir} DESTINATION share/orbital/elements FILES_MATCHING PATTERN "*.qml" PATTERN "element" PATTERN "*.js")

    if(LRELEASE_EXECUTABLE)
        file(GLOB _translations ${_dir}/*.ts)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_dir}")

        foreach(ts ${_translations})
            get_filename_component(_abs_ts ${ts} ABSOLUTE)
            get_filename_component(_name ${ts} NAME_WE)
            set(qm "${CMAKE_CURRENT_BINARY_DIR}/${_dir}/${_name}.qm")
            add_custom_command(OUTPUT ${qm} COMMAND ${LRELEASE_EXECUTABLE} ARGS ${_abs_ts} -qm ${qm} DEPENDS ${_abs} VERBATIM)

            list(APPEND ${_sources} "${qm}")
            install(FILES ${qm} DESTINATION share/orbital/${_dir})
        endforeach(ts)
        set(${_sources} ${${_sources}} PARENT_SCOPE)
    else(LRELEASE_EXECUTABLE)
        message(WARNING "Cannot find Qt's lrelease tool. Translations will not be generated")
    endif(LRELEASE_EXECUTABLE)
endfunction()

pkg_check_modules(WaylandClient wayland-client REQUIRED)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(Qt5LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
    style.cpp
    styleitem.cpp
    tooltip.cpp
    popup.cpp
    panel.cpp
    notification.cpp
    activeregion.cpp
    compositorsettings.cpp)

wayland_add_protocol_client(SOURCES
    ../../protocol/desktop-shell.xml
    desktop-shell
)
wayland_add_protocol_client(SOURCES
    ../../protocol/settings.xml
    settings
)

file(GLOB translations translations/*.ts)
qt5_add_resources(RESOURCES resources.qrc)
qt5_add_translation(QM_FILES ${translations})

install_element(SOURCES elements/background)
install_element(SOURCES elements/clock)
install_element(SOURCES elements/launcher)
install_element(SOURCES elements/logout)
install_element(SOURCES elements/mixer)
install_element(SOURCES elements/overlay)
install_element(SOURCES elements/pager)
install_element(SOURCES elements/panel)
install_element(SOURCES elements/runner)
install_element(SOURCES elements/taskbar)
install_element(SOURCES elements/deviceexplorer)
install_element(SOURCES elements/lockscreen)
install_element(SOURCES elements/systray)
install_element(SOURCES elements/battery)

list(APPEND defines "LIBRARIES_PATH=\"${CMAKE_INSTALL_PREFIX}/lib/orbital\"")
list(APPEND defines "DATA_PATH=\"${CMAKE_INSTALL_PREFIX}/share/orbital\"")
list(APPEND defines "LIBEXEC_PATH=\"${CMAKE_INSTALL_PREFIX}/libexec\"")

add_executable(orbital-client ${SOURCES} ${RESOURCES} ${QM_FILES})
qt5_use_modules(orbital-client Widgets Qml Quick)
target_link_libraries(orbital-client wayland-client)
set_target_properties(orbital-client PROPERTIES COMPILE_DEFINITIONS "${defines}")

install(TARGETS orbital-client DESTINATION libexec)
install(DIRECTORY styles DESTINATION share/orbital)
install(FILES ${QM_FILES} DESTINATION share/orbital/translations)

add_subdirectory(services/mixer)
add_subdirectory(services/login)
add_subdirectory(services/processlauncher)
add_subdirectory(services/datetime)
add_subdirectory(services/hardware)
add_subdirectory(services/notifications)
add_subdirectory(services/statusnotifier)
add_subdirectory(services/mpris)
