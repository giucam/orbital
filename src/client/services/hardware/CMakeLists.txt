
option(use_solid "Use the Solid library, if found." 1)

find_package(Qt5Core)
find_package(Qt5Qml)
if (${use_solid} MATCHES 1)
    find_package(KF5Solid QUIET)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

if(${KF5Solid_FOUND})
    get_property(include TARGET KF5::Solid PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${include})

    set(SOURCES hardwareservice.cpp)

    add_library(hardwareservice SHARED ${SOURCES})
    qt5_use_modules(hardwareservice Core Qml)
    target_link_libraries(hardwareservice KF5::Solid)
    install(TARGETS hardwareservice DESTINATION lib/orbital/services)
elseif (${use_solid} MATCHES 1)
    message("\nWARNING: Cannot find KF5 Solid. You can download it from https://projects.kde.org/projects/frameworks/solid/repository\n")
endif()
