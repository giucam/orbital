configure_file(restricted_interfaces.conf.in ${CMAKE_CURRENT_BINARY_DIR}/restricted_interfaces.conf @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/restricted_interfaces.conf DESTINATION /etc/orbital)
