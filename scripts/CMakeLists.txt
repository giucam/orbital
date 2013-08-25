
configure_file(startorbital.in ${CMAKE_CURRENT_BINARY_DIR}/startorbital @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startorbital DESTINATION ${CMAKE_INSTALL_PREFIX}/libexec)

configure_file(orbital.in ${CMAKE_CURRENT_BINARY_DIR}/orbital @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/orbital DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
