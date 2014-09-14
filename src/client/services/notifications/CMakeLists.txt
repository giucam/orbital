
find_package(Qt5Core)
find_package(Qt5DBus)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES notificationsservice.cpp notificationsadaptor.cpp)

add_library(notificationsservice SHARED ${SOURCES})
qt5_use_modules(notificationsservice Core DBus)
set_target_properties(notificationsservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
install(TARGETS notificationsservice DESTINATION lib/orbital/services)
