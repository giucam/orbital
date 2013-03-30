cmake_minimum_required(VERSION 2.8)
project(shell)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "-Wall -g -std=c++0x")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Wayland)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/
                    /usr/include/pixman-1)

set(SOURCES
    src/shell.cpp
    src/shellsurface.cpp
    src/effect.cpp
    src/scaleeffect.cpp
    src/desktop-shell.cpp)

wayland_add_protocol_server(SOURCES
    ${CMAKE_SOURCE_DIR}/protocol/desktop-shell.xml
    desktop-shell
)

add_library(shell SHARED ${SOURCES})
# target_link_libraries(shell )

install_targets(/lib/weston/ shell)
