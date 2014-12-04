cmake_minimum_required(VERSION 2.8)
project(orbital)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -g -std=c++0x -Werror=return-type")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Wayland)
find_package(PkgConfig "0.22" REQUIRED)

add_subdirectory(src)

# uninstall target
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

add_subdirectory(scripts)
