
option(use_solid "Use the Solid library, if found." 1)

find_package(Qt5Core)
find_package(Qt5Qml)
if (${use_solid} MATCHES ON)
    find_package(KF5Solid)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(SOURCES hardwareservice.cpp clibackend.cpp)

if(${KF5Solid_FOUND})
    get_property(include TARGET KF5::Solid PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${include})
    list(APPEND SOURCES solidbackend.cpp)
    list(APPEND defines "USE_SOLID")
    list(APPEND libs KF5::Solid)
elseif (${use_solid} MATCHES ON)
    message("WARNING: Cannot find KF5 Solid. You can download it from https://projects.kde.org/projects/frameworks/solid/repository\n")
endif()

add_library(hardwareservice SHARED ${SOURCES})
target_link_libraries(hardwareservice ${libs} Qt5::Core Qt5::Qml)
set_target_properties(hardwareservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
set(dest lib/orbital/qml/Orbital/HardwareService)
install(TARGETS hardwareservice DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
