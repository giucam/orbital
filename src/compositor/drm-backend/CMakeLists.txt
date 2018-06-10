
find_package(Qt5Core)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. /usr/include/libdrm)

set(SOURCES drm-backend.cpp)

add_library(drm-backend SHARED ${SOURCES})
target_link_libraries(drm-backend Qt5::Cor)
install(TARGETS drm-backend DESTINATION lib/orbital/compositor/backends)
