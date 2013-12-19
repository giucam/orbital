
find_package(Qt5Core)
find_package(Qt5Qml)
find_package(KF5Solid QUIET)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

if(${KF5Solid_FOUND})
    get_property(include TARGET KF5::Solid PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${include})

    set(SOURCES hardwareservice.cpp)

    add_library(hardwareservice SHARED ${SOURCES})
    qt5_use_modules(hardwareservice Core Qml)
    target_link_libraries(hardwareservice KF5::Solid)
    install(TARGETS hardwareservice DESTINATION lib/orbital/services)
else()
    message("\nWARNING: Cannot find KF5 Solid. You can download it from https://projects.kde.org/projects/frameworks/solid/repository\n")
endif()
