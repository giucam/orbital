
find_package(Qt5Core)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(SOURCES x11-backend.cpp)

add_library(x11-backend SHARED ${SOURCES})
target_link_libraries(x11-backend Qt5::Core)
install(TARGETS x11-backend DESTINATION lib/orbital/compositor/backends)
