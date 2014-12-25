
find_package(Qt5Core)
find_package(Qt5DBus)
find_package(Qt5Qml)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES statusnotifierservice.cpp
            statusnotifierwatcher.cpp
            statusnotifieritem.cpp
            statusnotifiericonprovider.cpp)

add_library(statusnotifierservice SHARED ${SOURCES})
qt5_use_modules(statusnotifierservice Core DBus Qml Gui Quick)
set_target_properties(statusnotifierservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
install(TARGETS statusnotifierservice DESTINATION lib/orbital/services)
