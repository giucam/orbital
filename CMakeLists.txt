cmake_minimum_required(VERSION 2.8)
project(orbital)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "-Wall -g -std=c++0x")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Wayland)
find_package(PkgConfig "0.22" REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})

add_subdirectory(src)
