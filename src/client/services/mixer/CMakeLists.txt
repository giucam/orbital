
find_package(Qt5Core)
find_package(ALSA)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(SOURCES mixerservice.cpp)
if (${ALSA_FOUND})
    list(APPEND SOURCES alsamixer.cpp)
endif()

add_library(mixerservice SHARED ${SOURCES})
qt5_use_modules(mixerservice Core)

if (${ALSA_FOUND})
    include_directories("${ALSA_INCLUDE_DIR}")
    list(APPEND defines "HAVE_ALSA")
    target_link_libraries(mixerservice ${ALSA_LIBRARIES})
else()
    message("\nCannot find Alsa\n")
endif()

set_target_properties(mixerservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
install(TARGETS mixerservice DESTINATION lib/orbital/services)
