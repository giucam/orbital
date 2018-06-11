
find_package(Qt5Core)
find_package(Qt5DBus)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set(SOURCES notificationsservice.cpp notificationsadaptor.cpp notificationsiconprovider.cpp)

add_library(notificationsservice SHARED ${SOURCES})
target_link_libraries(notificationsservice Qt5::Core Qt5::DBus Qt5::Quick)
set_target_properties(notificationsservice PROPERTIES COMPILE_DEFINITIONS "${defines}")
set(dest lib/orbital/qml/Orbital/NotificationsService)
install(TARGETS notificationsservice DESTINATION ${dest})
install(FILES qmldir DESTINATION ${dest})
