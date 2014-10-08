
option(use_logind "Use Logind to poweroff/reboot the computer." 1)

find_package(Qt5Core)
if (${use_logind} MATCHES ON)
    find_package(Qt5DBus)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES loginservice.cpp clibackend.cpp)

if (${use_logind} MATCHES ON)
    list(APPEND SOURCES logindbackend.cpp)
    list(APPEND defines "USE_LOGIND")
endif()

add_library(loginservice SHARED ${SOURCES})
qt5_use_modules(loginservice Core DBus Qml)
set_target_properties(loginservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
target_link_libraries(loginservice pam)
install(TARGETS loginservice DESTINATION lib/orbital/services)
