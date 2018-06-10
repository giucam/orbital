
option(use_pulseaudio "Use Pulseaudio, if found." 1)
option(use_alsa "Use the Alsa library, if found." 1)

find_package(Qt5Core)
if (${use_alsa} MATCHES ON)
    find_package(ALSA)
endif()
if (${use_pulseaudio} MATCHES ON)
    pkg_check_modules(pulse libpulse)
    pkg_check_modules(pulse-glib libpulse-mainloop-glib)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(SOURCES mixerservice.cpp)
if (${ALSA_FOUND})
    list(APPEND SOURCES alsamixer.cpp)
endif()

if (${pulse_FOUND})
    list(APPEND SOURCES pulseaudiomixer.cpp)
endif()

add_library(mixerservice SHARED ${SOURCES})

target_link_libraries(mixerservice Qt5::Core Qt5::Qml)

if (${ALSA_FOUND})
    include_directories("${ALSA_INCLUDE_DIR}")
    list(APPEND defines "HAVE_ALSA")
    target_link_libraries(mixerservice ${ALSA_LIBRARIES})
elseif (${use_alsa} MATCHES ON)
    message("\nCannot find Alsa\n")
endif()

if (${pulse_FOUND})
    include_directories(${pulse-glib_INCLUDE_DIRS})
    list(APPEND defines "HAVE_PULSE")
    target_link_libraries(mixerservice ${pulse_LIBRARIES} ${pulse-glib_LIBRARIES})
elseif (${use_pulseaudio} MATCHES ON)
    message("\nCannot find Pulseaudio\n")
endif()

target_link_libraries(mixerservice)

set(dest lib/orbital/qml/Orbital/MixerService)
set_target_properties(mixerservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
install(TARGETS mixerservice DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
